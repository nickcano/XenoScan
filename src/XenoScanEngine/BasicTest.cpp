#include "BasicTest.h"
#include <iostream>

BasicTest::BasicTest(const std::string& _testName, const std::wstring &_testFileName)
	: testFileName(_testFileName), TestBase(_testName)
{}

bool BasicTest::runTest(const LuaEngineShPtr &engine)
{
	if (!engine->doFile(this->testFileName))
	{
		std::cerr << "Failed to run test: ";
		std::wcerr << this->testFileName << std::endl;
		return false;
	}
	return this->completeTest(engine);
}