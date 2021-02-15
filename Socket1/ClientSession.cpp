#include "stdafx.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"

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

	
	HANDLE handle = CreateIoCompletionPort(reinterpret_cast<HANDLE>(mSocket), GIocpManager->GetCompletionPort(), 
		reinterpret_cast<ULONG_PTR>(this), 0);

	if (handle != GIocpManager->GetCompletionPort())
	{
		std::cout << "[DEBUG] CreateIoCompletionPort error: " << GetLastError() << '\n';
		return false;
	}

	memcpy(&mClientAddr, addr, sizeof(SOCKADDR_IN));
	mConnected = true;
	std::cout << "[DEBUG] Client Connected. Port:"<< ntohs(mClientAddr.sin_port) << '\n';

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

	if(SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_LINGER, reinterpret_cast<char*>(&lingerOption), sizeof(LINGER)))
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

	// TODO : WSARecv 사용

	return true;
}

bool ClientSession::PostSend(const char* buf, int len) const
{
	if (!IsConnected()) return false;
	OverlappedIOContext* sendContext = new OverlappedIOContext(this, IO_SEND);
	memcpy_s(sendContext->mBuffer, BUF_SIZE, buf, len);
	// TODO : WSASend 사용
	return true;
}