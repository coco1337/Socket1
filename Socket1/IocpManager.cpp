#include "stdafx.h"
#include "IocpManager.h"

#include "ClientSession.h"
#include "Server.h"
#include "SessionManager.h"

__declspec(thread) int LIoThreadId = 0;
IocpManager* GIocpManager = nullptr;

LPFN_ACCEPTEX IocpManager::mFnAcceptEx = nullptr;
LPFN_DISCONNECTEX IocpManager::mFnDisconnectEx = nullptr;
char IocpManager::mAcceptBuf[64] = { 0, };

BOOL DisconnectEx(SOCKET hSocket, LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD reserved)
{
	return IocpManager::mFnDisconnectEx(hSocket, lpOverlapped, dwFlags, reserved);
}

/*
BOOL AcceptEx(SOCKET sListenSocket, SOCKET sAcceptSocket, PVOID lpOutputBuffer, DWORD dwReceiveDataLength,
	DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived, LPOVERLAPPED lpOverlapped)
{
	return IocpManager::mFnAcceptEx(sListenSocket, sAcceptSocket, lpOutputBuffer, dwReceiveDataLength,
		dwLocalAddressLength, dwRemoteAddressLength, lpdwBytesReceived, lpOverlapped);
}
*/

IocpManager::IocpManager() : mCompletionPort(nullptr), mIoThreadCount(2), mListenSocket(NULL) {}
IocpManager::~IocpManager() {}

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

	// Create TCP socket
	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (mListenSocket == NULL) return false;

	HANDLE handle = CreateIoCompletionPort(reinterpret_cast<HANDLE>(mListenSocket), mCompletionPort, 0, 0);
	if (handle != mCompletionPort)
	{
		std::cout << "[DEBUG] listen socket IOCP Register error: " << GetLastError() << '\n';
		return false;
	}

	int opt = 1;
	setsockopt(
		mListenSocket,
		SOL_SOCKET,
		SO_REUSEADDR,
		reinterpret_cast<const char*>(&opt),
		sizeof(int));
	
	
	/*opt = 1;
	setsockopt(
		mListenSocket,
		SOL_SOCKET,
		SO_CONDITIONAL_ACCEPT,
		reinterpret_cast<char*>(&opt),
		sizeof(int));*/
	
	
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_ADDR, &(serveraddr.sin_addr));
	serveraddr.sin_port = htons(SERVER_PORT);

	// socket bind
	if (SOCKET_ERROR == bind(mListenSocket, 
		reinterpret_cast<SOCKADDR*>(&serveraddr), 
		sizeof(serveraddr))) return false;

	GUID guidDisconnectEx = WSAID_DISCONNECTEX;
	DWORD bytes = 0;
	if (SOCKET_ERROR == WSAIoctl(mListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidDisconnectEx, sizeof(GUID), &mFnDisconnectEx, 
		sizeof(LPFN_DISCONNECTEX), &bytes, nullptr, nullptr)) return false;

	GUID guidAcceptEx = WSAID_ACCEPTEX;
	if (SOCKET_ERROR == WSAIoctl(mListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx, sizeof(GUID), &mFnAcceptEx, 
		sizeof(LPFN_ACCEPTEX), &bytes, nullptr, nullptr)) return false;
	
	// make session pool
	GSessionManager->PrepareSessions();
	return true;
}

bool IocpManager::StartIoThreads()
{
	for (int i = 0; i < mIoThreadCount; ++i)
	{
		// making worker thread
		DWORD dwThreadId;
		HANDLE hThread = reinterpret_cast<HANDLE>(_beginthreadex(
			nullptr,
			0,
			IoWorkerThread,
			reinterpret_cast<LPVOID>(i + 1),
			0,
			reinterpret_cast<unsigned*>(&dwThreadId)));

		if (hThread == nullptr) return false;
	}

	return true;
}

bool IocpManager::StartAccept()
{
	// listen
	if (SOCKET_ERROR == listen(mListenSocket, SOMAXCONN))
	{
		std::cout << "[DEBUG] listen Error" << '\n';
		return false;
	}

	while (GSessionManager->AcceptSessions())
	{
		Sleep(100);
	}
}

void IocpManager::Finalize()
{
	CloseHandle(mCompletionPort);
	WSACleanup();
}

unsigned int WINAPI IocpManager::IoWorkerThread(LPVOID lpParam)
{
	// worker thread
	LThreadType = THREAD_IO_WORKER;
	LIoThreadId = reinterpret_cast<int>(lpParam);

	HANDLE hCompletionPort = GIocpManager->GetCompletionPort();

	while (true)
	{
		// GQCS
		DWORD dwTransferred = 0;
		OverlappedIOContext* context = nullptr;
		ULONG_PTR completionKey = 0;
		
		// Waiting for GQCS
		int ret = GetQueuedCompletionStatus(hCompletionPort,
			&dwTransferred,
			reinterpret_cast<PULONG_PTR>(&completionKey),
			reinterpret_cast<LPOVERLAPPED*>(&context),
			GQCS_TIMEOUT);

		ClientSession* theClient = context ? context->mSessionObject : nullptr;

		// check time out		
		if (ret == 0 || dwTransferred == 0)
		{
			int gle = GetLastError();
			if (gle == WAIT_TIMEOUT) continue;
			if (context->mIoType == IO_RECV || context->mIoType==IO_SEND)
			{
				theClient->DisconnectRequest(DR_COMPLETION_ERROR);
				DeleteIoContext(context);
				continue;
			}
		}

		bool completionOk = false;
		switch(context->mIoType)
		{
		case IO_DISCONNECT:
			theClient->DisconnectCompletion(reinterpret_cast<OverlappedDisconnectContext*>(context)->mDisconnectReason);
			completionOk = true;
			break;
		case IO_ACCEPT:
			theClient->AcceptCompletion();
			completionOk = true;
			break;
		case IO_RECV_ZERO:
			completionOk = PreReceiveCompletion(theClient, reinterpret_cast<OverlappedPreRecvContext*>(context), dwTransferred);
			break;
		case IO_SEND:
			completionOk = SendCompletion(theClient, reinterpret_cast<OverlappedSendContext*>(context), dwTransferred);
			break;
		case IO_RECV:
			completionOk = ReceiveCompletion(theClient, reinterpret_cast<OverlappedRecvContext*>(context), dwTransferred);
			break;
		default:
			std::cout << "Unknown I/O Type : " << context->mIoType << '\n';
			break;
		}

		if (!completionOk)
		{
			// connection closing
			theClient->DisconnectRequest(DR_IO_REQUEST_ERROR);
		}

		DeleteIoContext(context);
	}

	return 0;
}

bool IocpManager::PreReceiveCompletion(ClientSession* client, OverlappedPreRecvContext* context, DWORD dwTransferred)
{
	// real receive...
	return client->PostRecv();
}

bool IocpManager::ReceiveCompletion(ClientSession* client, OverlappedRecvContext* context, DWORD dwTransferred)
{
	client->RecvCompletion(dwTransferred);
	// echo back
	return client->PostSend();
}

bool IocpManager::SendCompletion(ClientSession* client, OverlappedSendContext* context, DWORD dwTransferred)
{
	client->SendCompletion(dwTransferred);

	if (context->mWsaBuf.len != dwTransferred)
	{
		std::cout << "Partial SendCompletion requested " << context->mWsaBuf.len << ", sent " << dwTransferred << '\n';
		return false;
	}

	//zero receive
	return client->PreRecv();
}
