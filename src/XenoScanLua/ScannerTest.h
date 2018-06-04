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
	uint64_t filetime64;
	uint32_t ticktime32;

	struct
	{
		uint32_t entries[7];
	} testStruct;
};