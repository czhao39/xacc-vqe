#include "ComputeEnergyVQETask.hpp"
#include "XACC.hpp"
#include "VQEProgram.hpp"

namespace xacc {
namespace vqe {

VQETaskResult ComputeEnergyVQETask::execute(
		Eigen::VectorXd parameters) {

	// Local Declarations
	auto comm = program->getCommunicator();
	double sum = 0.0, localExpectationValue = 0.0;
	int rank = comm.rank(), nlocalqpucalls = 0;
	int nRanks = comm.size();
	bool multiExec = false;

	auto statePrep = program->getStatePreparationCircuit();
	auto nQubits = program->getNQubits();
	auto qpu = program->getAccelerator();

	auto pauli = program->getPauliOperator();

	// Evaluate our variable parameterized State Prep circuite
	// to produce a state prep circuit with actual rotations
	auto evaluatedStatePrep = StatePreparationEvaluator::evaluateCircuit(
			statePrep, program->getNParameters(), parameters);

	auto kernels = program->getVQEKernels();

	VQETaskResult taskResult;

	if (xacc::optionExists("vqe-compute-energies-multi-exec")
			|| qpu->name() == "ibm") {
		multiExec = true;
	}

	if (multiExec) {

		xacc::info("Computing Energy with XACC Kernel Multi-Exec.");
		std::vector<double> coeffs;
		std::vector<Kernel<>> identityKernels;

		for (int i = 0; i < pauli.nTerms(); i++) {
			auto k = kernels[i];
			// If not identity
			if (k.getIRFunction()->nInstructions() > 0
					&& !boost::contains(k.getIRFunction()->getTag(),
							"readout-error")) {
				k.getIRFunction()->insertInstruction(0, evaluatedStatePrep);
				coeffs.push_back(
						std::real(
								boost::get<std::complex<double>>(
										k.getIRFunction()->getParameter(0))));
			} else {
				identityKernels.push_back(k);
			}
		}

		auto buff = qpu->createBuffer("qreg", nQubits);

		// FIXME, Goal here is to run AcceleratorBufferPostprocessors in KernelList.execute...
		auto tmpBuffers = kernels.execute(buff);
		nlocalqpucalls += tmpBuffers.size();

		int counter = 0;
		for (int i = 0; i < coeffs.size(); i++) {
			localExpectationValue = tmpBuffers[i]->getExpectationValueZ();
			xacc::info(
					"Fixed Expectation for Kernel " + std::to_string(counter) + " = "
							+ std::to_string(localExpectationValue));
			sum += coeffs[counter] * localExpectationValue;
			counter++;
		}

		for (auto k : identityKernels) {
			sum += std::real(
					boost::get<std::complex<double>>(
							k.getIRFunction()->getParameter(0)));
		}

		for (int i = 0; i < kernels.size(); i++) {
			if (kernels[i].getIRFunction()->nInstructions() > 0) {
				kernels[i].getIRFunction()->removeInstruction(0);
			}
		}
	} else {

		// Execute the kernels on the appropriate QPU
		// in parallel using OpenMP threads per
		// every MPI rank.
		int myStart = (rank) * kernels.size() / nRanks;
		int myEnd = (rank + 1) * kernels.size() / nRanks;
#pragma omp parallel for shared(kernels) reduction (+: sum, nlocalqpucalls)
		for (int i = myStart; i < myEnd; i++) {
			
			double lexpval = 1.0;
			
			// Get the ith Kernel
			auto kernel = kernels[i];

			auto t = std::real(boost::get<std::complex<double>>(
                               kernel.getIRFunction()->getParameter(0)));

			// If not an identity kernel...
			if (kernel.getIRFunction()->nInstructions() > 0) {
				// Insert the state preparation circuit IR
				// at location 0 in this Kernels IR instructions.
				kernel.getIRFunction()->insertInstruction(0,
						evaluatedStatePrep);

				// Create a temporary buffer of qubits
				auto buff = qpu->createBuffer("qreg", nQubits);

				// Execute the kernel!
				kernel(buff);
				nlocalqpucalls++;

				lexpval = buff->getExpectationValueZ();

//				taskResult.data[kernel.getIRFunction()->getName()].push_back(lexpval);
				xacc::info(
						"Fixed Expectation for Kernel " + std::to_string(i) + " = "
								+ std::to_string(lexpval));

				// The next iteration will have a different
				// state prep circuit, so toss the current one.
				kernel.getIRFunction()->removeInstruction(0);
			}

			// Sum up the expectation values, the Hamiltonian
			// terms coefficient is stored in the first
			// parameter of the Kernels IR Function representation
			sum += t * lexpval;
		}
	}

	double result = 0.0;
	int totalqpucalls = 0;
	boost::mpi::all_reduce(comm, sum, result, std::plus<double>());
	boost::mpi::all_reduce(comm, nlocalqpucalls, totalqpucalls, std::plus<double>());

	double currentEnergy = result;
	totalQpuCalls += totalqpucalls;

	std::stringstream ss;
	ss << std::setprecision(10) << currentEnergy << " at (" << parameters.transpose() << ")";

	if (rank == 0) {
		xacc::info("Iteration " + std::to_string(vqeIteration) + ", Computed VQE Energy = " + ss.str());
	}

	vqeIteration++;

	taskResult.results.push_back({parameters, currentEnergy});

	taskResult.energy = currentEnergy;
	taskResult.angles = parameters;
	taskResult.nQpuCalls = totalQpuCalls;

	return taskResult;
}


}
}
