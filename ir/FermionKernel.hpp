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
#ifndef QUANTUM_AQC_FermionKERNEL_HPP_
#define QUANTUM_AQC_FermionKERNEL_HPP_

#include "Registry.hpp"
#include "Function.hpp"
#include "FermionInstruction.hpp"
#include "XACC.hpp"

namespace xacc {
namespace vqe {

/**
 * The FermionKernel is an XACC Function that represents
 * a sum of FermionInstructions.
 */
class FermionKernel: public virtual Function {

protected:

	/**
	 * List of FermionInstructions
	 */
	std::list<InstPtr> instructions;

	/**
	 * This function's name
	 */
	std::string name;

public:

	/**
	 * The constructor, takes the function unique id and its name.
	 *
	 * @param id
	 * @param name
	 */
	FermionKernel(std::string kernelName) : name(kernelName) {
	}

	/**
	 * Return the number of FermionInstructions in this sum.
	 *
	 * @return
	 */
	virtual const int nInstructions() {
		return instructions.size();
	}

	/**
	 * Return the FermionInstruction at the given index.
	 *
	 * @param idx The index of the SpinInstruction
	 * @return instruction
	 */
	virtual InstPtr getInstruction(const int idx) {
		if (instructions.size() > idx) {
			return *std::next(instructions.begin(), idx);
		} else {
			xacc::error("Invalid instruction index.");
		}
	}

	virtual void mapBits(std::vector<int> bitMap) {
	}

	/**
	 * Return all FermionInstruction
	 *
	 * @return instructions
	 */
	virtual std::list<InstPtr> getInstructions() {
		return instructions;
	}

	/**
	 * Remove the instruction at the given index.
	 *
	 * @param idx The index of the instruction to remove.
	 */
	virtual void removeInstruction(const int idx) {
		instructions.remove(getInstruction(idx));
	}

	/**
	 * Add an instruction to this quantum
	 * intermediate representation.
	 *
	 * @param instruction
	 */
	virtual void addInstruction(InstPtr instruction) {
		instructions.push_back(instruction);
	}

	/**
	 * Replace the given current quantum instruction
	 * with the new replacingInst quantum Instruction.
	 *
	 * @param currentInst
	 * @param replacingInst
	 */
	virtual void replaceInstruction(const int idx, InstPtr replacingInst) {
		std::replace(instructions.begin(), instructions.end(),
				getInstruction(idx), replacingInst);
	}

	/**
	 * Insert the given FermionInstruction at the given index.
	 *
	 * @param idx The index of the new instruction
	 * @param newInst
	 */
	virtual void insertInstruction(const int idx, InstPtr newInst) {
		auto iter = std::next(instructions.begin(), idx);
		instructions.insert(iter, newInst);
	}

	/**
	 * Return the name of this function
	 * @return
	 */
	virtual const std::string getName() {
		return name;
	}

	/**
	 * Return the qubits this function acts on.
	 * @return
	 */
	virtual const std::vector<int> bits() {
		return std::vector<int> { };
	}

	/**
	 * Return an assembly-like string representation for this function .
	 * @param bufferVarName
	 * @return
	 */
	virtual const std::string toString(const std::string& bufferVarName) {
		std::stringstream ss;
		for (auto i : instructions) {
			ss << i->toString("") << " + \n";
		}
		return ss.str().substr(0, ss.str().size() - 3);
	}

	/**
	 * FermionKernel do not contain parameters. This
	 * method is not implmeneted. An error will be thrown if invoked.
	 *
	 * @param idx
	 * @return
	 */

	virtual InstructionParameter getParameter(const int idx) const{
		xacc::error("FermionKernel does not contain runtime parameters.");
	}

	/**
	 * FermionKernel do not contain parameters. This
	 * method is not implmeneted. An error will be thrown if invoked.
	 *
	 * @param idx
	 * @return
	 */

	virtual void setParameter(const int idx, InstructionParameter& p) {
		xacc::error("FermionKernel does not contain runtime parameters.");
	}

	/**
	 * FermionKernel do not contain parameters. This
	 * method is not implmeneted. An error will be thrown if invoked.
	 *
	 * @param idx
	 * @return
	 */

	virtual std::vector<InstructionParameter> getParameters() {
		xacc::error("FermionKernel does not contain runtime parameters.");
	}

	virtual void addParameter(InstructionParameter instParam) {
		xacc::error("FermionKernel does not contain runtime parameters.");
	}
	/**
	 * FermionKernel is not parameterized.
	 */
	virtual bool isParameterized() {
		return false;
	}

	virtual const std::string getTag() {
		return "";
	}

	/**
	 * FermionKernel has 0 parameters.
	 * @return
	 */
	virtual const int nParameters() {
		return 0;
	}

	/**
	 * FermionKernel do not contain parameters. This
	 * method is not implmeneted. An error will be thrown if invoked.
	 *
	 * @param idx
	 * @return
	 */
	virtual void evaluateVariableParameters(std::vector<InstructionParameter> parameters) {
		xacc::error("FermionKernel does not contain runtime parameters.");
	}


	EMPTY_DEFINE_VISITABLE()

};

}
}

#endif
