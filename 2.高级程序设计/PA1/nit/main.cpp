#include <iostream>
#include "nit.h"


int main(int argc, char* argv[])
{
	if (argc < 2) 
	{
		std::cout << "No command provided." << std::endl;
		return 1;
	}

	std::string command = argv[1];
	Nit nit;

	if (command == "init") 
	{
		nit.Init();
	}
	else if (command == "add")
	{
		if (argc != 3) 
		{
			std::cout << "Usage: nit add [filename]" << std::endl;
			return 1;
		}

		nit.LoadStagineArea();
		nit.Add(argv[2]);
	}
	else if (command == "commit")
	{
		if (argc != 3) 
		{
			std::cout << "Please enter a commit message" << std::endl;
			return 1;
		}

		nit.LoadStagineArea();
		nit.Commit(argv[2]);
	}
	else if (command == "status") 
	{
		nit.LoadStagineArea();
		nit.LoadRemoveFileArea();
		nit.Status();
	}
	else if (command == "checkout") 
	{
		if (argc != 3) 
		{
			std::cout << "Usage: nit checkout [commit id]" << std::endl;
			return 1;
		}

		nit.Checkout(argv[2]);
	}
	else if (command == "log") 
	{
		nit.LoadAllCommit();
		nit.Log();
	}
	else 
	{
		std::cout << "Unknown command: " << command.c_str() << std::endl;
	}

	return 0;
}
