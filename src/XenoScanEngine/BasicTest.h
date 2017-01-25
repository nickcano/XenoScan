#pragma once
#include "TestBase.h"

#include <string>


class BasicTest : TestBase
{
public:
	BasicTest(const std::string& _testName, const std::wstring &_testFileName);
	virtual ~BasicTest() {}

	virtual bool runTest(const LuaEngineShPtr &engine);

private:
	std::wstring testFileName;

};