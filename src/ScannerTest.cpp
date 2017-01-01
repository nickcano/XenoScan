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

	// structure stuff
	this->testStruct.one = 999;
	this->testStruct.two = 1234;
	this->testStruct.three = 0xDEADBEEF;
	this->testStruct.four = 0xBADF00D;
	this->testStruct.five = 4321;
	this->testStruct.six = 1000000;
	this->testStruct.seven = 7654321;

	engine->pushGlobal("TEST_STRUCT_ONE", this->testStruct.one);
	engine->pushGlobal("TEST_STRUCT_TWO", this->testStruct.two);
	engine->pushGlobal("TEST_STRUCT_THREE", this->testStruct.three);
	engine->pushGlobal("TEST_STRUCT_FOUR", this->testStruct.four);
	engine->pushGlobal("TEST_STRUCT_FIVE", this->testStruct.five);
	engine->pushGlobal("TEST_STRUCT_SIX", this->testStruct.six);
	engine->pushGlobal("TEST_STRUCT_SEVEN", this->testStruct.seven);
	engine->pushGlobal("TEST_STRUCT_ADDRESS", &this->testStruct);


	if (!engine->doFile(L"ScannerTest.lua"))
	{
		std::cerr << "Failed to run test: ScannerTest.lua" << std::endl;
		return false;
	}
	return this->completeTest(engine);
}