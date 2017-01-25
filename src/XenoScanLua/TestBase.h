#pragma once
#include "LuaEngine.h"

#include <list>


class TestBase
{
public:
	TestBase(const std::string& _testName);
	virtual ~TestBase();
	virtual bool runTest(const LuaEngineShPtr &engine) = 0;

	static void runAllTests(const LuaEngineShPtr &engine);

protected:
	virtual bool completeTest(const LuaEngineShPtr &engine);

private:
	std::string testName;

	static std::list<TestBase*> tests;

	// disallow copy and assign
	TestBase(const TestBase&);
	void operator=(const TestBase&);
};

