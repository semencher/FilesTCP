#pragma once

#include <Windows.h>

class Client
{
public:
	Client();
	void start();
	void stop();
	void sendFile(const std::string &fileName);

private:
	static DWORD WINAPI clientThreadStatic(void *param);
	void clientThread();

private:
	bool forTerminateThread_;
	HANDLE threadH_;
	DWORD threadDW_;
	
	std::string fileName_;

};	// class Client