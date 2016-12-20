#include <iostream>

#include "LuaEngine.h"
#include "ScannerTarget.h"
#include "Scanner.h"


int main(int argv, char** argc)
{
	LuaEngineShPtr eng(new LuaEngine());

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


	return 0;
}