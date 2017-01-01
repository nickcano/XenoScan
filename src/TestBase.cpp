#include "TestBase.h"

#include <algorithm>
#include <iostream>

TestBase::TestBase(const std::string& _testName)
	: testName(_testName)
{
	TestBase::tests.push_back(this);
}

TestBase::~TestBase()
{
	std::remove(TestBase::tests.begin(), TestBase::tests.end(), this);
}

bool TestBase::completeTest(const LuaEngineShPtr &engine)
{
	bool result;
	LuaVariant status;
	if (!engine->getGlobal("TEST_STATUS", status) || !status.getAsBool(result))
	{
		std::cerr << "    [ERR] TEST_STATUS variable not defined!" << std::endl;
		return false;
	}

	if (!result)
	{
		LuaVariant messages;
		LuaVariant::LuaVariantITable errorMessages;
		if (!engine->getGlobal("TEST_MESSAGES", messages) || !messages.getAsITable(errorMessages))
			std::cerr << "    [ERR] Unknown reason for test failure!" << std::endl;
		else
		{
			for (auto msg = errorMessages.begin(); msg != errorMessages.end(); msg++)
			{
				std::string message;
				if (msg->getAsString(message))
					std::cerr << "    [ERR] " << message << std::endl;
			}
		}
		return false;
	}

	return true;
}

void TestBase::runAllTests(const LuaEngineShPtr &engine)
{
	size_t failures = 0;
	for (auto t = TestBase::tests.begin(); t != TestBase::tests.end(); t++)
	{
		std::cout << "Running test '" << (*t)->testName << "'... " << std::endl;
		if ((*t)->runTest(engine))
			std::cout << "Success!";
		else
		{
			std::cout << "Fail!";
			failures++;
		}
		std::cout << std::endl;
	}

	std::cout << "Total test failures: " << failures << std::endl;
}
