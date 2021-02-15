#include "IocpManager.h"
#include "Server.h"
#include "SessionManager.h"

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
	if (mCompletionPort == nullptr) return false;

	mListenSocket = WSASocketW(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (mListenSocket == NULL) return false;

	int opt = 1;
	setsockopt(
		mListenSocket,
		SOL_SOCKET,
		SO_REUSEADDR,
		reinterpret_cast<const char*>(&opt),
		sizeof(int));

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
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
		HANDLE hThread = (HANDLE)_beginthreadex(
			nullptr,
			0,
			IoWorkerThread,
			reinterpret_cast<LPVOID>(i + 1),
			0,
			reinterpret_cast<unsigned*>(&dwThreadId));

		if (hThread == nullptr) return false;
	}

	return true;
}

unsigned int WINAPI IocpManager::IoWorkerThread(LPVOID lpParam)
{
	LThreadType = THREAD_IO_WORKER;
	LIoThreadId = reinterpret_cast<int>(lpParam);

	HANDLE hCompletionPort = GIocpManager->GetCompletionPort();

	while (true)
	{
		DWORD dwTransferred = 0;
		OverlappedIOContext* context = nullptr;
		ClientSession* asCompletionKey = nullptr;

		int ret = GetQueuedCompletionStatus(hCompletionPort,
			&dwTransferred,
			(PULONG_PTR)&asCompletionKey,
			(LPOVERLAPPED*)&context,
			GQCS_TIMEOUT);

		// check timeout first
		if (ret == 0 && GetLastError() == WAIT_TIMEOUT) continue;

		if (ret == 0 && dwTransferred == 0)
		{
			// connection closing
			asCompletionKey->Disconnect(DR_RECV_ZERO);
			GSessionManager->DeleteClientSession(asCompletionKey);
			continue;
		}

		if (nullptr == context)
		{
			std::cout << "error\n";
			continue;
		}

		bool completionOk = true;
		switch(context->mIoType)
		{
		case IO_SEND:
			completionOk = SendCompletion(asCompletionKey, context, dwTransferred);
			break;
		case IO_RECV:
			completionOk = ReceiveCompletion(asCompletionKey, context, dwTransferred);
			break;
		default:
			std::cout << "Unknown I/O Type: " << context->mIoType << '\n';
			break;
		}

		if (!completionOk)
		{
			// connection closing
			asCompletionKey->Disconnect(DR_COMPLETION_ERROR);
			GSessionManager->DeleteClientSession(asCompletionKey);
		}
	}

	return 0;
}


bool IocpManager::StartAcceptLoop()
{
	// listen
	if (SOCKET_ERROR == listen(mListenSocket, SOMAXCONN)) return false;

	// accept loop
	while (true)
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

		ClientSession* client = GSessionManager->CreateClientSession(acceptedSock);

		if (false == client->OnConnect(&clientaddr))
		{
			client->Disconnect(DR_ONCONNECT_ERROR);
			GSessionManager->DeleteClientSession(client);
		}
	}

	return true;
}

void IocpManager::Finalize()
{
	CloseHandle(mCompletionPort);
	WSACleanup();
}

bool IocpManager::ReceiveCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred)
{
	delete context;
	return client->PostRecv();
}

bool IocpManager::SendCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred)
{
	delete context;
	return true;
}