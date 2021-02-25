#include "stdafx.h"
#include "Server.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"

OverlappedIOContext::OverlappedIOContext(ClientSession* owner, IOType ioType)
	: mSessionObject(owner), mIoType(ioType)
{
	memset(&mOverlapped, 0, sizeof(OVERLAPPED));
	memset(&mWsaBuf, 0, sizeof(WSABUF));
	mSessionObject->AddRef();
}

ClientSession::ClientSession() : mBuffer(BUF_SIZE), mConnected(0), mRefCount(0)
{
	memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));
	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
}

void ClientSession::SessionReset()
{
	mConnected = 0;
	mRefCount = 0;
	memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));

	mBuffer.BufferReset();

	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	// no TCP TIME_WAIT
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_LINGER, reinterpret_cast<char*>(&lingerOption), sizeof(LINGER)))
	{
		std::cout << "[DEBUG] setsockopt linger option error: " << GetLastError() << '\n';
	}
	closesocket(mSocket);

	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
}

bool ClientSession::PostAccept()
{
	OverlappedAcceptContext* acceptContext = new OverlappedAcceptContext(this);
	// AcceptEx로 구현
	
	return true;
}


void ClientSession::AcceptCompletion()
{
	if (1 == InterlockedExchange(&mConnected, 1))
	{
		// already exists?
		return;
	}

	bool resultOk = true;
	do
	{
		if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, 
			reinterpret_cast<char*>(GIocpManager->GetListenSocket()), sizeof(SOCKET)))
		{
			printf_s("[DEBUG] SO_UPDATE_ACCEPT_CONTEXT error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

		int opt = 1;
		if (SOCKET_ERROR == setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, 
			reinterpret_cast<const char*>(&opt), sizeof(int)))
		{
			printf_s("[DEBUG] TCP_NODELAY error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

		opt = 0;
		if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, 
			reinterpret_cast<const char*>(&opt), sizeof(int)))
		{
			printf_s("[DEBUG] SO_RCVBUF change error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

		int addrlen = sizeof(SOCKADDR_IN);
		if (SOCKET_ERROR == getpeername(mSocket, reinterpret_cast<SOCKADDR*>(&mClientAddr), &addrlen))
		{
			printf_s("[DEBUG] getpeername error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

		//TODO: CreateIoCompletionPort를 이용한 소켓 연결
		//HANDLE handle = CreateIoCompletionPort(...);

	} while (false);

	if (!resultOk)
	{
		DisconnectRequest(DR_ONCONNECT_ERROR);
		return;
	}

	std::cout << "[DEBUG] Client Connected. port : " << ntohs(mClientAddr.sin_port) << '\n';

	if (false == PreRecv())
	{
		std::cout << "[DEBUG] PreRecv error : " << GetLastError() << '\n';
	}
}


void ClientSession::DisconnectRequest(DisconnectReason dr)
{
	// 이미 끊겼거나 끊기는 중이거나
	if (0 == InterlockedExchange(&mConnected, 0)) return;

	OverlappedDisconnectContext* context = new OverlappedDisconnectContext(this, dr);

	// TODO : DisconnectEx를 이용한 연결 끊기 요청
}


void ClientSession::DisconnectCompletion(DisconnectReason dr)
{
	std::cout << "[DEBUG] Client Disconnected : Reason = " << dr << ", port = " << ntohs(mClientAddr.sin_port) << '\n';

	// release refcount when added at issuing a session
	ReleaseRef();
}

bool ClientSession::PreRecv()
{
	if (!IsConnected()) return false;

	OverlappedPreRecvContext* recvContext = new OverlappedPreRecvContext(this);

	// zero-byte recv 구현

	return true;
}

bool ClientSession::PostRecv()
{
	if (!IsConnected()) return false;

	FastSpinlockGuard cs(mBufferLock);

	if (0 == mBuffer.GetFreeSpaceSize()) return false;

	OverlappedRecvContext* recvContext = new OverlappedRecvContext(this);

	DWORD recvbytes = 0;
	DWORD flags = 0;
	recvContext->mWsaBuf.len = static_cast<ULONG>(mBuffer.GetFreeSpaceSize());
	recvContext->mWsaBuf.buf = mBuffer.GetBuffer();

	// start recv
	if (SOCKET_ERROR == WSARecv(mSocket, &recvContext->mWsaBuf, 1, 
		&recvbytes, &flags, reinterpret_cast<LPWSAOVERLAPPED>(recvContext), nullptr))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(recvContext);
			std::cout << "ClientSession::PostRecv Error : " << GetLastError() << '\n';
			return false;
		}
	}

	return true;
}

void ClientSession::RecvCompletion(DWORD transferred)
{
	FastSpinlockGuard cs(mBufferLock);
	mBuffer.Commit(transferred);
}

bool ClientSession::PostSend()
{
	if (!IsConnected()) return false;

	FastSpinlockGuard cs(mBufferLock);

	if (0 == mBuffer.GetContinuousBytes()) return true;

	OverlappedSendContext* sendContext = new OverlappedSendContext(this);

	DWORD sendbytes = 0;
	DWORD flags = 0;
	sendContext->mWsaBuf.len = static_cast<ULONG>(mBuffer.GetContinuousBytes());
	sendContext->mWsaBuf.buf = mBuffer.GetBufferStart();

	// start async send
	if (SOCKET_ERROR == WSASend(mSocket, &sendContext->mWsaBuf, 1, &sendbytes, flags,
		reinterpret_cast<LPWSAOVERLAPPED>(sendContext), nullptr))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(sendContext);
			std::cout << "ClientSession::PostSend Error : " << GetLastError() << '\n';
			return false;
		}
	}

	return true;
}

void ClientSession::SendCompletion(DWORD transferred)
{
	FastSpinlockGuard cs(mBufferLock);

	mBuffer.Remove(transferred);
}

void ClientSession::AddRef()
{
	
}

void ClientSession::ReleaseRef()
{
	long ret = InterlockedDecrement(&mRefCount);

	if (ret == 0) GSessionManager->ReturnClientSession(this);
}

void DeleteIoContext(OverlappedIOContext* context)
{
	if (nullptr == context) return;
	context->mSessionObject->ReleaseRef();
	delete context;
}




