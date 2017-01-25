#pragma once
#include "TestBase.h"
#include <stdint.h>

class ScannerTest : TestBase
{
public:
	ScannerTest();
	virtual ~ScannerTest() {}

	virtual bool runTest(const LuaEngineShPtr &engine);


private:
	char string1[32];
	std::string string2;
	std::wstring string3;
	struct
	{
		uint32_t one;
		uint32_t two;
		uint32_t three;
		uint32_t four;
		uint32_t five;
		uint32_t six;
		uint32_t seven;
	} testStruct;
};