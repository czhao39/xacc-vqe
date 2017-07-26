#include "JordanWignerIRTransformation.hpp"

namespace xacc {
namespace vqe {

std::shared_ptr<IR> JordanWignerIRTransformation::transform(
		std::shared_ptr<IR> ir) {

	// We assume we have a FermionIR instance, which contains
	// one FermionKernel, which contains N FermionInstructions, one
	// for each term in the hamiltonian.

	// We want to map that to a Hamiltonian composed of pauli matrices
	// But, we want each term of that to be a separate IR Function.

	// Create a new GateQIR to hold the spin based terms
	auto newIr = std::make_shared<xacc::quantum::GateQIR>();

	auto fermiKernel = ir->getKernels()[0];

	int counter = 1;
	CompositeSpinInstruction total;

	// Loop over all Fermionic terms...
	for (auto fermionInst : fermiKernel->getInstructions()) {

		// Get the creation or annihilation sites
		auto termSites = fermionInst->bits();

		// Get the params indicating if termSite is creation or annihilation
		auto params = fermionInst->getParameters();

		auto fermionCoeff = boost::get<std::complex<double>>(params[termSites.size()]);

		CompositeSpinInstruction current;
		for (int i = 0; i < termSites.size(); i++) {
			// Create Zi's
			std::vector<std::pair<int, std::string>> zs;
			for (int j = 0; j <= termSites[i]-1; j++) {
				zs.push_back({j, "Z"});
			}

			SpinInstruction zSpins(zs);
			CompositeSpinInstruction sigPlusMinus;
			auto xi = std::make_shared<SpinInstruction>(
					std::vector<std::pair<int, std::string>> { { termSites[i],
							"X" } });
			sigPlusMinus.addInstruction(xi);

			if (boost::get<int>(params[i])) { // If this is a creation operator
				// Create sigmaPlus
				auto iyi = std::make_shared<SpinInstruction>(
						std::vector<std::pair<int, std::string>> { {
								termSites[i], "Y" } },
						std::complex<double>(0, -1));
				sigPlusMinus.addInstruction(iyi);
			} else {
				auto negiyi = std::make_shared<SpinInstruction>(
						std::vector<std::pair<int, std::string>> { {
								termSites[i], "Y" } },
						std::complex<double>(0, 1));
				sigPlusMinus.addInstruction(negiyi);
			}

			auto temp = 0.5 * sigPlusMinus;
			temp = zSpins * temp;

			if (i == 0) {
				current = temp;
			} else {
				current = current * temp;
			}
		}

		auto temp = fermionCoeff * current;
		result = result + temp;

	}

	// Populate GateQIR now...

	return newIr;
}

CompositeSpinInstruction JordanWignerIRTransformation::getResult() {
	return result;
}

}
}

