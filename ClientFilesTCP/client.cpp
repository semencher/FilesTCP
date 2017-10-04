#include <iostream>
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
	while (!forTerminateThread_)
	{
		bool forDelete = false;
		char *buffer;
		if (fileName_ != "") {
			buffer = new char[fileName_.length() + 1];
			std::strcpy(buffer, fileName_.c_str());
			fileName_ = "";
			forDelete = true;
		}
		else {
			buffer = "Hello from client!\n";
		}

		int buf_size = (int)strlen(buffer);
		int sent = 0;
		while (sent < buf_size)
		{
			result = send(clientSocket, buffer + sent, (int)strlen(buffer), 0);
			if (result == SOCKET_ERROR)
			{
				printf("send from client failed with error: %d\n", WSAGetLastError());
				break;
			}
			sent += result;
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