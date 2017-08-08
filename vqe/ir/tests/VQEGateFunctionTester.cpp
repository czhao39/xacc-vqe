
/***********************************************************************************
 * Copyright (c) 2016, UT-Battelle
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
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE VQEGateFunctionTester

#include <boost/test/included/unit_test.hpp>
#include "VQEGateFunction.hpp"
#include "Rz.hpp"

using namespace xacc::vqe;

BOOST_AUTO_TEST_CASE(checkFermionKernelConstruction) {

	xacc::InstructionParameter p("0.5 * phi");
	xacc::InstructionParameter fp("phi");

	auto rz = std::make_shared<Rz>(std::vector<int>{0});
	rz->setParameter(0, p);

	VQEGateFunction f("foo", std::vector<xacc::InstructionParameter>{fp}, 3.0);

	f.addInstruction(rz);

	std::cout << f.toString("qreg") << "\n";

	xacc::InstructionParameter runtimeValue(3.1415);

	f.evaluateVariableParameters(std::vector<xacc::InstructionParameter>{runtimeValue});

	BOOST_VERIFY(boost::get<double>(f.getInstruction(0)->getParameter(0)) == 1.57075);

	std::cout << "ParamSet:\n" << f.toString("qreg") << "\n";

	xacc::InstructionParameter runtimeValue2(6.28);
	f.evaluateVariableParameters(std::vector<xacc::InstructionParameter>{runtimeValue2});
	std::cout << "ParamSet:\n" << f.toString("qreg") << "\n";
	BOOST_VERIFY(boost::get<double>(f.getInstruction(0)->getParameter(0)) == 3.14);

}
