#pragma once

#include <Windows.h>

class Client
{
public:
	Client();
	void start();
	void stop();

private:
	static DWORD WINAPI clientThreadStatic(void *param);
	void clientThread();

private:
	bool forTerminateThread_;
	HANDLE threadH_;
	DWORD threadDW_;

};	// class Client