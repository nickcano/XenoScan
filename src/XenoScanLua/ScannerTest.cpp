#include "ScannerTest.h"
#include <iostream>

#include <Windows.h>

ScannerTest::ScannerTest()
	: TestBase("Scanner")
{}

bool ScannerTest::runTest(const LuaEngineShPtr &engine)
{
	// TODO: this needs to be fixed for non-windows compilation
	engine->pushGlobal("TEST_PID", (uint32_t)GetCurrentProcessId());

	// string stuff
	strcpy_s<32>(this->string1, "I'm a really cool string brotha");
	this->string2 = "Teeeeeeest. String.";
	this->string3 = L"Test. Wide. String. Foobar?";

	engine->pushGlobal("TEST_STRING1", this->string1);
	engine->pushGlobal("TEST_STRING2", this->string2);
	engine->pushGlobal("TEST_STRING3", this->string3);

	engine->pushGlobal("TEST_STRING1_ADDRESS", &this->string1);
	engine->pushGlobal("TEST_STRING2_ADDRESS", (void*)this->string2.data());
	engine->pushGlobal("TEST_STRING3_ADDRESS", (void*)this->string3.data());

	// structure stuff
	this->testStruct.entries[0] = 999;
	this->testStruct.entries[1] = 1234;
	this->testStruct.entries[2] = 0xDEADBEEF;
	this->testStruct.entries[3] = 0xBADF00D;
	this->testStruct.entries[4] = 4321;
	this->testStruct.entries[5] = 1000000;
	this->testStruct.entries[6] = 7654321;

	engine->pushGlobal("TEST_STRUCT_ONE", this->testStruct.entries[0]);
	engine->pushGlobal("TEST_STRUCT_TWO", this->testStruct.entries[1]);
	engine->pushGlobal("TEST_STRUCT_THREE", this->testStruct.entries[2]);
	engine->pushGlobal("TEST_STRUCT_FOUR", this->testStruct.entries[3]);
	engine->pushGlobal("TEST_STRUCT_FIVE", this->testStruct.entries[4]);
	engine->pushGlobal("TEST_STRUCT_SIX", this->testStruct.entries[5]);
	engine->pushGlobal("TEST_STRUCT_SEVEN", this->testStruct.entries[6]);
	engine->pushGlobal("TEST_STRUCT_ADDRESS", &this->testStruct.entries[0]);


	if (!engine->doFile(L"ScannerTest.lua"))
	{
		std::cerr << "Failed to run test: ScannerTest.lua" << std::endl;
		return false;
	}
	return this->completeTest(engine);
}