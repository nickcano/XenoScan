#include "TestBase.h"
#include "BasicTest.h"


// this must be defined first because things coming later
// will be inserted into it
std::list<TestBase*> TestBase::tests;


// Defining a test in global scope will automatically
// cause it to be run when tests are run.
BasicTest typeSystemTests("Lua Type System", L"TypeSystemTest.lua");