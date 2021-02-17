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

	// Create Completion port
	mCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (mCompletionPort == nullptr) return false;

	// Create Listen socket
	mListenSocket = WSASocketW(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (mListenSocket == NULL) return false;

	int opt = 1;
	setsockopt(
		mListenSocket,
		SOL_SOCKET,
		SO_REUSEADDR,
		(const char*)&opt,
		sizeof(int));

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_ADDR, &(serveraddr.sin_addr));
	serveraddr.sin_port = htons(SERVER_PORT);

	// socket bind
	if (SOCKET_ERROR == bind(mListenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr))) return false;

	return true;
}

bool IocpManager::StartIoThreads()
{
	for (int i = 0; i < mIoThreadCount; ++i)
	{
		// making worker thread
		DWORD dwThreadId;
		HANDLE hThread = (HANDLE)_beginthreadex(
			nullptr,
			0,
			IoWorkerThread,
			(LPVOID)(i + 1),
			0,
			(unsigned*)&dwThreadId);

		if (hThread == nullptr) return false;
	}

	return true;
}

unsigned int WINAPI IocpManager::IoWorkerThread(LPVOID lpParam)
{
	// worker thread
	LThreadType = THREAD_IO_WORKER;
	LIoThreadId = reinterpret_cast<int>(lpParam);

	HANDLE hCompletionPort = GIocpManager->GetCompletionPort();
	int ret;

	while (true)
	{
		// ���� ���鼭 GQCS�� �Ϸ� ���� ���������� ���
		DWORD dwTransferred = 0;
		OverlappedIOContext* context = nullptr;
		ClientSession* asCompletionKey = nullptr;

		ret = GetQueuedCompletionStatus(hCompletionPort,
			&dwTransferred,
			(PULONG_PTR)&asCompletionKey,
			(LPOVERLAPPED*)&context,
			GQCS_TIMEOUT);

		// check time out
		if (ret == 0 && GetLastError() == WAIT_TIMEOUT) continue;
		
		if (ret == 0 || dwTransferred == 0)
		{
			asCompletionKey->Disconnect(DR_RECV_ZERO);
			GSessionManager->DeleteClientSession(asCompletionKey);
			continue;
		}
		

		if (nullptr == context)
		{
			std::cout << "nullptr == context\n";
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

	// accept loop, AcceptEx�� �ٲٱ�
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
		getpeername(acceptedSock, (SOCKADDR*)&clientaddr, &addrlen);

		// ���� ���� ����ü �Ҵ�, �ʱ�ȭ
		ClientSession* client = GSessionManager->CreateClientSession(acceptedSock);

		// Ŭ���̾�Ʈ ���� ó��
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
	return true;
}

bool IocpManager::SendCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred)
{
	delete context;
	return true;
}