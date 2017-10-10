#include <iostream>
#include <string>
#include <fstream>
#include <winsock2.h>
#include <WS2tcpip.h>

#include "client.h"

Client::Client()
{
	threadH_ = nullptr;
}

void Client::start()
{
	forTerminateThread_ = false;
	// 4 параметр, это указатель на переменную передаваемую потоку.
	threadH_ = CreateThread(nullptr, 0, &clientThreadStatic, this, 0, &threadDW_);
}

void Client::stop()
{
	forTerminateThread_ = true;
	if (threadH_)
	{
		WaitForSingleObject(threadH_, INFINITE);
		CloseHandle(threadH_);
		threadH_ = nullptr;
	}
}

DWORD WINAPI Client::clientThreadStatic(void *param)
{
	Client *client = (Client*)param;
	client->clientThread();
	return 0;
}

void Client::sendFile(const std::string &fileName)
{
	fileName_ = fileName;
}

#define SERVER_PORT "21345"
#define SERVER_ADDR "localhost"

void Client::clientThread()
{
	std::cout << "Client has been started.\n";
	// Структура для хранения адреса хоста.
	struct addrinfo *serverAddr = nullptr;
	// Структура различна в зависимости от выбранного протокола.
	struct sockaddr_in clientAddr;
	struct addrinfo hints;
	SOCKET clientSocket = INVALID_SOCKET;	// Это недопустимое значение соккета.
	int result;

	// Функция для заполнения области памяти нулями.
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;	// Указывает на тип IPv4.
	hints.ai_socktype = SOCK_STREAM;	// Тип соккета с надежным подключением TCP.
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;	// При намерении использовать вызывающую структуру в bind.
	
	WSADATA wsaData;
	int iResult;
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return;
	}

	// Resolve the server address and port
	result = getaddrinfo(SERVER_ADDR, SERVER_PORT, &hints, &serverAddr);
	if (result != 0)
	{
		std::cout << "getaddrinfo failed with error: " << result << "\n";
		return;
	}

	clientAddr = *(sockaddr_in*)serverAddr->ai_addr;
	clientAddr.sin_port = 0;

	// Create a SOCKET for connecting to server
	clientSocket = socket(serverAddr->ai_family, serverAddr->ai_socktype, serverAddr->ai_protocol);
	if (clientSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(serverAddr);
		return;
	}

	// Setup the TCP listening socket
	result = bind(clientSocket, (sockaddr*)&clientAddr, sizeof(clientAddr));
	if (result == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(serverAddr);
		closesocket(clientSocket);
		return;
	}
	result = connect(clientSocket, serverAddr->ai_addr, (int)serverAddr->ai_addrlen);
	if (result == SOCKET_ERROR)
	{
		printf("connect failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(serverAddr);
		closesocket(clientSocket);
		return;
	}
	freeaddrinfo(serverAddr);
	bool sendName = false;
	while (!forTerminateThread_)
	{
		bool forDelete = false;
		char *buffer = nullptr;
		if (!fileName_.empty()) {

			const size_t sizeBuffer = 400;

			std::ifstream iFile;
			iFile.open(fileName_, std::ios::in | std::ios::binary);

			iFile.seekg(0, std::ios::end);
			size_t length = iFile.tellg();
			iFile.seekg(0, std::ios::beg);

			buffer = new char[sizeBuffer + 1];

			if (!sendName) {
				int i = 0;
				for (; i < fileName_.size(); ++i) {
					buffer[i] = fileName_[i];
				}
				buffer[i] = '*';
				++i;
				std::string size = std::to_string(length);
				for (int j = 0; j < size.size(); ++j, ++i) {
					buffer[i] = size[j];
				}
				buffer[i] = '*';
				buffer[i + 1] = '\0';
				++i;

				result = send(clientSocket, buffer, i, 0);
				sendName = true;
			}


			int sent = 0;
			while (length > sizeBuffer) {
				iFile.read(buffer, sizeBuffer);
				buffer[sizeBuffer] = '\0';
				result = send(clientSocket, buffer, sizeBuffer, 0);
				if (result == SOCKET_ERROR)
				{
					printf("send from client failed with error: %d\n", WSAGetLastError());
					std::cout << sent;
					break;
				}
				sent += result;
				length = length - sizeBuffer;
			}
			if (length > 0) {
				iFile.read(buffer, length);
				buffer[length] = '\0';
				result = send(clientSocket, buffer, length, 0);
				if (result == SOCKET_ERROR)
				{
					printf("send from client failed with error: %d\n", WSAGetLastError());
					break;
				}
				sent += result;
			}

			fileName_ = "";
			forDelete = true;
			sendName = false;
		}

		Sleep(1000);
		if (forDelete)
			delete[]buffer;
	}
	result = shutdown(clientSocket, SD_SEND);
	if (result == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
	}
	closesocket(clientSocket);

	std::cout << "Client has been stopped.\n";
	WSACleanup();
}