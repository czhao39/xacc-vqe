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
#include <gtest/gtest.h>
#include "BravyiKitaevIRTransformation.hpp"
#include "XACC.hpp"

using namespace xacc::vqe;


std::shared_ptr<FermionKernel> compileKernel(const std::string src) {
	// First off, split the string into lines
	std::vector<std::string> lines, fLineSpaces;
	boost::split(lines, src, boost::is_any_of("\n"));
	auto functionLine = lines[0];
//	std::cout << "HELLO WORLD: " << functionLine << "\n";
	boost::split(fLineSpaces, functionLine, boost::is_any_of(" "));
	auto fName = fLineSpaces[1];
	boost::trim(fName);
	fName = fName.substr(0, fName.find_first_of("("));
	auto firstCodeLine = lines.begin() + 1;
	auto lastCodeLine = lines.end() - 1;
	std::vector<std::string> fermionStrVec(firstCodeLine, lastCodeLine);

	auto fermionKernel = std::make_shared<FermionKernel>("fName");

	int _nQubits = 0;
	// Loop over the lines to create DWQMI
	for (auto termStr : fermionStrVec) {
		boost::trim(termStr);
		if (!termStr.empty() && (std::string::npos != termStr.find_first_of("0123456789"))) {
			std::vector<std::string> splitOnSpaces;
			boost::split(splitOnSpaces, termStr, boost::is_any_of(" "));

			// We know first term is coefficient
			// FIXME WHAT IF COMPLEX
			auto coeff = std::stod(splitOnSpaces[0]);
			std::vector<std::pair<int, int>> operators;
			for (int i = 1; i < splitOnSpaces.size()-1; i+=2) {
				auto siteIdx = std::stoi(splitOnSpaces[i]);
				if (siteIdx > _nQubits) {
					_nQubits = siteIdx;
				}
				operators.push_back(
						{siteIdx, std::stoi(
								splitOnSpaces[i + 1]) });
			}

			auto fermionInst = std::make_shared<FermionInstruction>(operators,
					coeff);
			fermionKernel->addInstruction(fermionInst);
		}
	}

	_nQubits++;

	xacc::setOption("n-qubits", "4");
	return fermionKernel;
}

TEST(BravyiKitaevIRTransformationTester,checkBKTransform) {

	xacc::setOption("n-qubits", "3");
	auto Instruction = std::make_shared<FermionInstruction>(
			std::vector<std::pair<int, int>> { { 2, 1 }, { 0, 0 }}, 3.17);
	auto Instruction2 = std::make_shared<FermionInstruction>(
			std::vector<std::pair<int, int>> { { 0, 1 }, { 2, 0 }}, 3.17);

	auto kernel = std::make_shared<FermionKernel>("foo");
	kernel->addInstruction(Instruction);
	kernel->addInstruction(Instruction2);

	BravyiKitaevIRTransformation bkTransform;

	auto result = bkTransform.transform(*kernel);

	std::cout << "HI: " << result.toString() << "\n";

	PauliOperator expected({{0,"Y"}, {1,"Y"}}, -1.585);
	expected += PauliOperator({{0,"X"}, {1,"X"}, {2,"Z"}}, -1.585);

	EXPECT_TRUE(expected == result);

}

TEST(BravyiKitaevIRTransformationTester,checkH2Transform) {

	const std::string code =
			R"code(__qpu__ kernel() {
   0.7137758743754461
   -1.252477303982147 0 1 0 0
   0.337246551663004 0 1 1 1 1 0 0 0
   0.0906437679061661 0 1 1 1 3 0 2 0
   0.0906437679061661 0 1 2 1 0 0 2 0
   0.3317360224302783 0 1 2 1 2 0 0 0
   0.0906437679061661 0 1 3 1 1 0 2 0
   0.3317360224302783 0 1 3 1 3 0 0 0
   0.337246551663004 1 1 0 1 0 0 1 0
   0.0906437679061661 1 1 0 1 2 0 3 0
   -1.252477303982147 1 1 1 0
   0.0906437679061661 1 1 2 1 0 0 3 0
   0.3317360224302783 1 1 2 1 2 0 1 0
   0.0906437679061661 1 1 3 1 1 0 3 0
   0.3317360224302783 1 1 3 1 3 0 1 0
   0.3317360224302783 2 1 0 1 0 0 2 0
   0.0906437679061661 2 1 0 1 2 0 0 0
   0.3317360224302783 2 1 1 1 1 0 2 0
   0.0906437679061661 2 1 1 1 3 0 0 0
   -0.4759344611440753 2 1 2 0
   0.0906437679061661 2 1 3 1 1 0 0 0
   0.3486989747346679 2 1 3 1 3 0 2 0
   0.3317360224302783 3 1 0 1 0 0 3 0
   0.0906437679061661 3 1 0 1 2 0 1 0
   0.3317360224302783 3 1 1 1 1 0 3 0
   0.0906437679061661 3 1 1 1 3 0 1 0
   0.0906437679061661 3 1 2 1 0 0 1 0
   0.3486989747346679 3 1 2 1 2 0 3 0
   -0.4759344611440753 3 1 3 0
})code";

	auto fermionKernel = compileKernel(code);

	xacc::setOption("n-qubits", "4");

	BravyiKitaevIRTransformation t;
	auto result = t.transform(*fermionKernel);

	PauliOperator expected({{0,"Z"}, {1,"Z"}, {2,"Z"}}, .165868);
	expected += PauliOperator({{1,"Z"}, {3,"Z"}}, .174349);
	expected += PauliOperator({{1,"Z"}, {2,"Z"}, {3,"Z"}}, -.222796);
	expected += PauliOperator({{0,"Z"}, {1,"Z"}, {2,"Z"}, {3,"Z"}}, .165868);
	expected += PauliOperator({{0,"Z"}, {2,"Z"}}, .120546);
	expected += PauliOperator({{1,"Z"}}, 0.168623);
	expected += PauliOperator({{0,"Z"}, {1,"Z"}}, .171201);
	expected += PauliOperator({{2,"Z"}}, -.222796);
	expected += PauliOperator({{0,"Z"}}, .171201);
	expected += PauliOperator({{0,"Z"}, {2,"Z"}, {3,"Z"}}, 0.120546);

	expected += PauliOperator({}, -.0988349);
	expected += PauliOperator({{0,"X"}, {1,"Z"}, {2,"X"}, {3,"Z"}}, 0.0453219);
	expected += PauliOperator({{0,"X"}, {1,"Z"}, {2,"X"}}, 0.0453219);
	expected += PauliOperator({{0,"Y"}, {1,"Z"}, {2,"Y"}}, 0.0453219);
	expected += PauliOperator({{0,"Y"}, {1,"Z"}, {2,"Y"}, {3,"Z"}}, 0.0453219);

	EXPECT_TRUE(expected == result);


}

int main(int argc, char** argv) {
   xacc::Initialize(argc,argv);
   ::testing::InitGoogleTest(&argc, argv);
   auto ret = RUN_ALL_TESTS();
   xacc::Finalize();
   return ret;
}
