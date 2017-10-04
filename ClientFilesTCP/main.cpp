#include <iostream>
#include <string>

#include "client.h"

#pragma comment (lib, "Ws2_32.lib")

int main()
{
	Client client;
	client.start();
	
	std::string command;
	while (true)
	{
		std::cout << "\n>";
		std::getline(std::cin, command);
		if (command == "quit")
			break;
		else
			client.sendFile(command);
	}

	client.stop();
}