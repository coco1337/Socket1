#pragma once
#include "stdafx.h"

// iocp ����� ���õ� �Լ���

class IocpManager
{
public:
	IocpManager() = default;
	~IocpManager() = default;
	
	bool Initialize();
	void Finalize();

	bool StartIoThreads();
	bool StartAcceptLoop();

	HANDLE GetCompletionPort() { return mCompletionPort; }
	int GetIoThreadCount() { return mIoThreadCount; }

private:
	static unsigned int WINAPI IoWorkerThread(LPVOID lpParam);
	
	
	int mIoThreadCount;
	HANDLE mCompletionPort;
	SOCKET mListenSocket;
};

extern __declspec(thread) int LIoThreadId;
extern IocpManager* GIocpManager;