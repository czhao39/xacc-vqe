/***********************************************************************************
 * Copyright (c) 2017, UT-Battelle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the xacc nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Contributors:
 *   Initial API and implementation - Alex McCaskey
 *
 **********************************************************************************/
#ifndef VQE_IR_JORDANWIGNERIRTRANSFORMATION_HPP_
#define VQE_IR_JORDANWIGNERIRTRANSFORMATION_HPP_

#include "FermionToSpinTransformation.hpp"

namespace xacc {

namespace vqe {

/**
 * The JordanWignerIRTransformation is a realization of the XACC
 * IRTransformation interface that provides a transform method
 * that takes as input a FermionIR instance and transforms it to
 * a GateQIR instance.
 *
 * Specifically, this transformation will generate N GateFunctions,
 * one for each term in the generated spin-based hamiltonian.
 */
class JordanWignerIRTransformation: public FermionToSpinTransformation {

public:

	/**
	 * Transform a FermionIR instance to a GateQIR instance.
	 *
	 * @param ir
	 * @return
	 */
	virtual std::shared_ptr<IR> transform(std::shared_ptr<IR> ir);
	virtual PauliOperator transform(FermionKernel& kernel);

	virtual const std::string name() const {
		return "jw";
	}

	virtual const std::string description() const {
		return "The Jordan-Wigner IR Transformation uses the Jordan-Wigner "
				"transformation to map fermionic instructions to spin-based instructions.";
	}

};

}

}

#endif
