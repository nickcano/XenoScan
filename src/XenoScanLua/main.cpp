#include <iostream>
#include <thread>
#include <atomic>

#include "LuaEngine.h"
#include "TestBase.h"

#include "XenoScanEngine/ScannerTarget.h"
#include "XenoScanEngine/Scanner.h"

void processInput(LuaEngineShPtr eng)
{
	// this is a bit messy but it gets the job done
	std::string block = "";
	std::atomic<bool> blockRead(false);
	auto readThread = std::thread([&] {
		while (true)
		{
			if (blockRead)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				continue;
			}

			std::string input = "";
			do
			{
				input = "";
				std::cout << ">>>";
				std::getline(std::cin, input);
				block += input;
				block += "\n";
			} while (input != "");
			blockRead = true;
		}
	});

	while (true)
	{
		eng->doThink();

		if (blockRead)
		{
			std::cout << "Executing block... " << std::endl;
			eng->doString(block);
			block = "";
			blockRead = false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

}


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
		for (int a = 1; a < argc; a++)
		{
			std::string path(argv[a]);
			eng->doFile(std::wstring(path.begin(), path.end()));
		}

		processInput(eng);
	}


	return 0;
}