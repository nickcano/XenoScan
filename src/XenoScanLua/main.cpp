#include <iostream>

#include "LuaEngine.h"
#include "TestBase.h"

#include <ScannerTarget.h>
#include <Scanner.h>


int main(int argc, char** argv)
{
	LuaEngineShPtr eng(new LuaEngine());

	if (argc == 2 && strcmp(argv[1], "test") == 0)
	{
		system("pause");
		TestBase::runAllTests(eng);
	}
	else
	{
		while (true)
		{
			std::string block = "", input = "";
			do
			{
				input = "";
				std::cout << ">>>";
				std::getline(std::cin, input);
				block += input;
				block += "\n";
			} while (input != "");
			std::cout << "Executing block... " << std::endl;
			eng->doString(block);
		}
	}


	return 0;
}