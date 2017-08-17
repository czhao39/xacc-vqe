#ifndef VQE_VQEPROBLEM_HPP_
#define VQE_VQEPROBLEM_HPP_

#include "problem.h"
#include "XACC.hpp"
#include <boost/math/constants/constants.hpp>

#include "VQEGateFunction.hpp"
#include "InstructionIterator.hpp"

namespace xacc {

namespace vqe {

template<typename T>
class VQEProblem : public cppoptlib::Problem<T> {

protected:

	std::shared_ptr<Accelerator> qpu;

	std::shared_ptr<Program> program;

	std::vector<Kernel<>> kernels;

	int nParameters;

	int nQubits;

public:

	using typename cppoptlib::Problem<T>::TVector;

	double currentEnergy;

	VQEProblem(std::istream& moleculeKernel) : nParameters(0), currentEnergy(0.0) {
		xacc::setCompiler("fermion");
		xacc::setOption("state-preparation", "uccsd");
		xacc::setOption("n-electrons", "2");

		// Create the Accelerator
		qpu = xacc::getAccelerator("tnqvm");

		// Create the Program
		program = std::make_shared<Program>(qpu, moleculeKernel);

		// Start compilation
		program->build();

		// Create a buffer of qubits
		nQubits = std::stoi(xacc::getOption("n-qubits"));

		// Get the Kernels that were created
		kernels = program->getRuntimeKernels();

		// Set the number of VQE parameters
		nParameters = kernels[0].getNumberOfKernelParameters();

		if (xacc::optionExists("vqe-print-scaffold-source")) {
			int counter = 0;
			for (auto k : kernels) {
				auto scaffold = xacc::getCompiler("scaffold");
				auto f = k.getIRFunction();
				auto srcStr = scaffold->translate("qreg", f);

				boost::filesystem::path dir("scaffold_source");
				if (!boost::filesystem::exists(dir)) {
					if (!boost::filesystem::create_directory(dir)) {
						XACCError(
								"Could not create scaffold_source directory.");
					}
				}
				std::ofstream out(
						"scaffold_source/"
								+ xacc::getOption("vqe-print-scaffold-source")
								+ "_kernel_" + std::to_string(counter) + ".hpp");
				out << srcStr;
				out.flush();
				out.close();
				counter++;
			}

			if (xacc::optionExists("vqe-exit-after-scaffold")) {
				xacc::Finalize();
				exit(0);
			}
		}
	}

	typename cppoptlib::Problem<T>::TVector initializeParameters() {
		std::srand(time(0));
		auto pi = boost::math::constants::pi<double>();
		// Random parameters between -pi and pi
		auto rand = -1.0 * pi * Eigen::VectorXd::Ones(nParameters)
				+ (Eigen::VectorXd::Random(nParameters) * 0.5
						+ Eigen::VectorXd::Ones(nParameters) * 0.5)
						* (pi - (-1 * pi));
		return rand;
	}


	T value(const TVector& x) {
		auto pi = boost::math::constants::pi<double>();
		std::vector<InstructionParameter> parameters;
		for (int i = 0; i < x.rows(); i++) {
			InstructionParameter p(x(i));
			parameters.push_back(p);
		}

		// Evaluate all parameters first,
		// since this invokes the Python Interpreter (for now)
		for (auto k : kernels) {
			k.evaluateParameters(parameters);
			InstructionIterator it(k.getIRFunction());
			while (it.hasNext()) {
				// Get the next node in the tree
				auto nextInst = it.next();
				nextInst->enable();
				auto gateName = nextInst->getName();
				if (gateName == "Rx" || gateName == "Rz") {
					auto angle =
							boost::get<double>(nextInst->getParameter(0));
					if (angle < 0.0) {
						InstructionParameter newParam(4 * pi + angle);
						nextInst->setParameter(0, newParam);
					}
				}
			}

/*
			auto statePrep = std::dynamic_pointer_cast<Function>(k.getIRFunction()->getInstruction(0));
			InstructionIterator it2(statePrep);
			it2.next();
			int counter = 0, index = 0;
			while (it2.hasNext()) {
				auto next = it2.next();
				if (next->getName() == "Rz"
						&& std::fabs(boost::get<double>(next->getParameter(0)))
								< 1e-12) {
//					std::cout << "Found an Rz with 0 angle\n";

//					next->disable();
					// If I'm here, I have counter instructions in front of me
					// So disable the counter in front and counter behind
					while(counter != 0) {
//					for (int i = index-counter; i <= 2*(counter+1); i++) {
//						std::cout << counter << ", " << index << " | Disabling " << statePrep->getInstruction(index-counter)->toString("qreg") << " and " << statePrep->getInstruction(index+counter)->toString("qreg") << "\n";
//						statePrep->getInstruction(index-counter)->disable();
//						statePrep->getInstruction(index+counter)->disable();
						counter--;
					}

					// reset the counter
//					counter = 0;
				}

				if (next->getName() != "X" && next->isEnabled()) {
					counter++;
				}

				index++;
			}*/
		}

		double sum = 0.0, localExpectationValue = 0.0;
		for (int i = 0; i < kernels.size(); i++) {

			// Get the ith Kernel
			auto kernel = kernels[i];

			// We need the reference to the IR Function
			// in order to get the leading coefficient
			auto vqeFunction = std::dynamic_pointer_cast<VQEGateFunction>(
							kernel.getIRFunction());
			double coeff = vqeFunction->coefficient;

			// We only need a temporary AcceleratorBuffer,
			// so just create it here for this thread's iteration
			auto buff = qpu->createBuffer("qreg", nQubits);

			// If we have instructions, execute the kernel
			// Execute!
			kernel(buff);

			// Get Expectation value
			if (vqeFunction->isIdentityOperator) {
				localExpectationValue = 1.0;
			} else {
				localExpectationValue = buff->getExpectationValueZ();
			}

			// Sum up the expectation values
			sum += coeff * localExpectationValue;
//			std::cout << vqeFunction->nInstructions() << " Kernel " << i << " Expectation = " << localExpectationValue << ", coeff = " << coeff << ": " << sum << "\n";

		}

		currentEnergy = sum;

		std::stringstream ss;
		ss << parameters[0] << " " << parameters[1];//x.transpose();
		XACCInfo("Computed VQE Energy = " + std::to_string(sum) + " at (" + ss.str() + ")");
		return sum;
	}

	class VQECriteria : public cppoptlib::Criteria<T> {
	public:
	    static VQECriteria defaults() {
	    	VQECriteria d;
	        d.iterations = 1000;
	        d.xDelta = 0;
	        d.fDelta = 1e-6;
	        d.gradNorm = 1e-4;
	        d.condition = 0;
	        return d;
	    }
	};


	static VQECriteria getConvergenceCriteria() {
		auto criteria = VQECriteria::defaults();

		if (xacc::optionExists("vqe-energy-delta")) {
			criteria.fDelta = std::stod(xacc::getOption("vqe-energy-delta"));
		}

		if (xacc::optionExists("vqe-iterations")) {
			criteria.iterations = std::stoi(xacc::getOption("vqe-iterations"));
		}

		return criteria;

	}

};
}

}

#endif
//
//auto scaffold = xacc::getCompiler("scaffold");
//	auto f = kernel.getIRFunction();
//	auto srcStr = scaffold->translate("qreg", f);
//
//	boost::filesystem::path dir("temp");
//	if (!boost::filesystem::exists(dir)) {
//		if (!boost::filesystem::create_directory(dir)) {
//			XACCError("Could not create scaffold_source directory.");
//		}
//	}
//	std::ofstream out("temp/kernel_" + std::to_string(i) + "_at_"
//					+ std::to_string(boost::get<double>(parameters[0])) + "_" + std::to_string(boost::get<double>(parameters[1]))
//					+ ".hpp");
//	out << srcStr;
//	out.flush();
//	out.close();
