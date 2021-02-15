#pragma once
#include "stdafx.h"
#include "ClientSession.h"

// iocp 연결과 관련된 함수들

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

	static bool SendCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred);
	static bool ReceiveCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred);
	
	
	int mIoThreadCount;
	HANDLE mCompletionPort;
	SOCKET mListenSocket;
};

extern __declspec(thread) int LIoThreadId;
extern IocpManager* GIocpManager;