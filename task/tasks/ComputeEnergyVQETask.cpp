#include "ComputeEnergyVQETask.hpp"
#include "XACC.hpp"
#include "VQEProgram.hpp"

namespace xacc {
namespace vqe {

VQETaskResult ComputeEnergyVQETask::execute(
		Eigen::VectorXd parameters) {

	// Local Declarations
	auto comm = program->getCommunicator();
	double sum = 0.0;
	int rank = comm->rank(), nlocalqpucalls = 0;
	int nRanks = comm->size();
	std::map<std::string, double> expVals, readoutProbs;
	bool persist = xacc::optionExists("vqe-persist-data");

	// Get info about the problem
	auto statePrep = program->getStatePreparationCircuit();
	auto nQubits = program->getNQubits();
	auto qpu = program->getAccelerator();

	// Evaluate our variable parameterized State Prep circuite
	// to produce a state prep circuit with actual rotations
	auto evaluatedStatePrep = StatePreparationEvaluator::evaluateCircuit(
			statePrep, program->getNParameters(), parameters);

	// Utility functions for readability
	auto isReadoutErrorKernel = [](const std::string& tag) -> bool 
		{ return boost::contains(tag,"readout-error");};
	auto getCoeff = [](Kernel<>& k) ->double {
		return std::real(boost::get<std::complex<double>>(k.getIRFunction()->getParameter(0)));
	};

	// Create an empty KernelList to be filled 
	// with non-trivial kernels
	KernelList<> kernels(qpu);
	//kernels.setBufferPostprocessors(program->getBufferPostprocessors());
	for (auto& k : program->getVQEKernels()) {
		if (k.getIRFunction()->nInstructions() > 0) { 
			// If not identity, add the state prep to the circuit
			if (k.getIRFunction()->getTag() != "readout-error") {
				k.getIRFunction()->insertInstruction(0,evaluatedStatePrep);
			}
			kernels.push_back(k);
		} else {
			// if it is identity, add its coeff to the energy sum
			if (rank == 0) sum += getCoeff(k);
		}
	}

	// Allocate some qubits
	auto buf = qpu->createBuffer("tmp",nQubits);

	// We can do this in parallel or serially
	if (xacc::optionExists("vqe-use-mpi")) {
		int myStart = (rank) * kernels.size() / nRanks;
		int myEnd = (rank + 1) * kernels.size() / nRanks;
		for (int i = myStart; i < myEnd; i++) {
			kernels[i](buf);
			totalQpuCalls++;
			if(!isReadoutErrorKernel(kernels[i].getIRFunction()->getTag())) {
				sum += getCoeff(kernels[i]) 
					* buf->getExpectationValueZ();
			}
			buf->resetBuffer();
		}

		double result = 0.0;
		int ncalls = 0;
		comm->sumDoubles(sum, result);
		comm->sumInts(totalQpuCalls, ncalls);
		totalQpuCalls = ncalls;
		sum = result;
	} else {
		// Execute all nontrivial kernels!
		auto results = kernels.execute(buf);
		totalQpuCalls += qpu->isRemote() ? 1 : kernels.size();

		// Compute the energy
		for(int i = 0; i < results.size(); ++i) {
			auto k = kernels[i];
			auto exp = results[i]->getExpectationValueZ();
			if(!isReadoutErrorKernel(k.getIRFunction()->getTag())) {
				sum += exp * 
					getCoeff(k);
			}
			if(k.getIRFunction()->getTag() != "readout-error") {
				expVals.insert({k.getName(), exp});
			} else if (xacc::optionExists("correct-readout-errors")) {
				auto name = k.getName(); // Qubit_{0,1}
				std::vector<std::string> split;
				boost::split(split, name, boost::is_any_of("_"));
				auto qbit = std::stoi(split[0]);
				auto zero_one = std::stoi(split[1]);
				std::string bitstr = "";
				for (int i = 0; i < nQubits; i++) bitstr += "0";
				if (zero_one == 0) bitstr[nQubits - qbit - 1] = '1';
				auto prob = results[i]->computeMeasurementProbability(bitstr);
				readoutProbs.insert({"p_"+name, std::isnan(prob) ? 0.0 : prob});
			}
		}

		// Clean up by removing the state prep
		// from the measurement kernels
		for(auto& k : kernels) k.getIRFunction()->removeInstruction(0);	
	}

	std::stringstream ss;
	ss << std::setprecision(10) << sum << " at (" << parameters.transpose() << ")";
	if (rank == 0) {
		xacc::info("Iteration " + std::to_string(vqeIteration) + ", Computed VQE Energy = " + ss.str());
	}

	vqeIteration++;

	// See if the user requested data persisitence
	if (persist) {
		VQETaskResult taskResult(xacc::getOption("vqe-persist-data"));
		taskResult.energy = sum;
		taskResult.angles = parameters;
		taskResult.nQpuCalls = totalQpuCalls;
		taskResult.expVals = expVals;
		taskResult.readoutErrorProbabilities = readoutProbs;
		taskResult.persist();
		return taskResult;
	} else {
		VQETaskResult taskResult;
		taskResult.energy = sum;
		taskResult.angles = parameters;
		taskResult.nQpuCalls = totalQpuCalls;
		taskResult.expVals = expVals;
		taskResult.readoutErrorProbabilities = readoutProbs;
		return taskResult;
	}
}

}
}
