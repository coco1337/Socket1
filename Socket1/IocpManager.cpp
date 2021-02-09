#include "IocpManager.h"
#include "Server.h"

__declspec(thread) int LIoThreadId = 0;
IocpManager* GIocpManager = nullptr;

bool IocpManager::Initialize()
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	mIoThreadCount = info.dwNumberOfProcessors;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

	mCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

	mListenSocket = WSASocketW(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);

	int opt = 1;
	setsockopt(
		mListenSocket, 
		SOL_SOCKET, 
		SO_REUSEADDR, 
		reinterpret_cast<const char*>(&opt), 
		sizeof(int));
	
	SOCKADDR_IN serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_ADDR, &(serveraddr.sin_addr));
	serveraddr.sin_port = htons(SERVER_PORT);

	if (SOCKET_ERROR == bind(mListenSocket, reinterpret_cast<SOCKADDR*>(&serveraddr), sizeof(serveraddr))) return false;

	return true;
}

bool IocpManager::StartIoThreads()
{
	for (int i = 0; i < mIoThreadCount; ++i)
	{
		DWORD dwThreadId;
		// HANDLE hThread = (HANDLE)_beginthreadex(nullptr, 0, )
	}

	return false;
}

unsigned int WINAPI IocpManager::IoWorkerThread(LPVOID lpParam)
{
	LThreadType = THREAD_IO_WORKER;
	LIoThreadId = reinterpret_cast<int>(lpParam);

	HANDLE hCompletionPort = GIocpManager->GetCompletionPort();

	while(true)
	{
		DWORD dwTransferred = 0;
	}

	return 0;
}


bool IocpManager::StartAcceptLoop()
{
	// listen
	if (SOCKET_ERROR == listen(mListenSocket, SOMAXCONN)) return false;

	// accept loop
	while(true)
	{
		auto acceptedSock = accept(mListenSocket, nullptr, nullptr);
		if (acceptedSock == INVALID_SOCKET)
		{
			std::cout << "Accept: invalid socket\n";
			continue;
		}

		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(acceptedSock, reinterpret_cast<SOCKADDR*>(&clientaddr), &addrlen);

		std::cout << clientaddr.sin_addr.S_un.S_addr << '\n';
	}
}

void IocpManager::Finalize()
{
	WSACleanup();
}