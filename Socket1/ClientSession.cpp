#include "stdafx.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"

OverlappedIOContext::OverlappedIOContext(const ClientSession* owner, IOType ioType)
	: mSessionObject(owner), mIoType(ioType)
{
	memset(&mOverlapped, 0, sizeof(OVERLAPPED));
	memset(mBuffer, 0, BUF_SIZE);
	memset(&mWsaBuf, 0, sizeof(WSABUF));
	mWsaBuf.buf = mBuffer;
	mWsaBuf.len = BUF_SIZE;
}

bool ClientSession::OnConnect(SOCKADDR_IN* addr)
{
	FastSpinlockGuard cs(mLock);
	// non-blocking
	u_long arg = 1;
	ioctlsocket(mSocket, FIONBIO, &arg);

	// turn off nagle
	int opt = 1;
	setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY,
		reinterpret_cast<const char*>(&opt), sizeof(int));

	opt = 0;
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&opt), sizeof(int)))
	{
		std::cout << "[DEBUG] SO_RCVBUF change error: " << GetLastError() << '\n';
		return false;
	}

	// IOCompletionPort에 소켓 연결
	HANDLE handle = CreateIoCompletionPort(reinterpret_cast<HANDLE>(mSocket),
		GIocpManager->GetCompletionPort(),
		reinterpret_cast<ULONG_PTR>(this),
		0);

	if (handle != GIocpManager->GetCompletionPort())
	{
		std::cout << "[DEBUG] CreateIoCompletionPort error: " << GetLastError() << '\n';
		return false;
	}

	memcpy(&mClientAddr, addr, sizeof(SOCKADDR_IN));
	mConnected = true;
	std::cout << "[DEBUG] Client Connected. Port:" << ntohs(mClientAddr.sin_port) << '\n';

	GSessionManager->IncreaseConnectionCount();

	return PostRecv();
}

void ClientSession::Disconnect(DisconnectReason dr)
{
	FastSpinlockGuard cs(mLock);
	if (!IsConnected()) return;

	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	if (SOCKET_ERROR == setsockopt(mSocket,
		SOL_SOCKET,
		SO_LINGER,
		reinterpret_cast<char*>(&lingerOption),
		sizeof(LINGER)))
	{
		std::cout << "[DEBUG] setsockopt linger option error: " << GetLastError() << '\n';
	}

	std::cout << "[DEBUG] Client Disconnected: Reason: " << dr << " Port: " << ntohs(mClientAddr.sin_port) << '\n';
	GSessionManager->DecreaseConnectionCount();
	closesocket(mSocket);
	mConnected = false;
}

bool ClientSession::PostRecv() const
{
	if (!IsConnected()) return false;
	OverlappedIOContext* recvContext = new OverlappedIOContext(this, IO_RECV);

	DWORD recvBytes = 0;
	DWORD flags = 0;

	if (SOCKET_ERROR == WSARecv(mSocket,
		&recvContext->mWsaBuf,
		1,
		&recvBytes,
		&flags,
		reinterpret_cast<LPWSAOVERLAPPED>(recvContext),
		nullptr))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			std::cout << "ClientSession::PostRecv Error: " << GetLastError() << '\n';
			return false;
		}
	}

	if (recvBytes > 0)
	{
		std::cout << "recvBytes::" << recvBytes << ", " << recvContext->mBuffer << '\n';
	}

	return true;
}

bool ClientSession::PostSend(const char* buf, int len) const
{
	if (!IsConnected()) return false;
	OverlappedIOContext* sendContext = new OverlappedIOContext(this, IO_SEND);
	memcpy_s(sendContext->mBuffer, BUF_SIZE, buf, len);
	DWORD sendCount = 0;
	
	if (SOCKET_ERROR == WSASend(mSocket,
		static_cast<LPWSABUF>(&sendContext->mWsaBuf),
		1,
		reinterpret_cast<LPDWORD>(&sendCount),
		0,
		static_cast<LPOVERLAPPED>(&sendContext->mOverlapped),
		nullptr))
	{
		std::cout << "WSASend error: " << GetLastError() << '\n';
	}
	return true;
}