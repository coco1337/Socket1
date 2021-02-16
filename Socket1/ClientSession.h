#pragma once
#include "FastSpinlock.h"

// 세션, 각종 이벤트들 -> OnConnect, Disconnect, PostSend, PostRecv 등등

class ClientSession;
class IocpManager;

enum IOType
{
	IO_NONE,
	IO_SEND,
	IO_RECV,
	IO_RECV_ZERO,
	IO_ACCEPT,
	IO_DISCONNECT
};

enum DisconnectReason
{
	DR_NONE,
	DR_RECV_ZERO,
	DR_ACTIVE,
	DR_ONCONNECT_ERROR,
	DR_COMPLETION_ERROR,
};

struct OverlappedIOContext
{
	OverlappedIOContext(const ClientSession* owner, IOType ioType);

	OVERLAPPED mOverlapped;
	const ClientSession* mSessionObject;
	IOType mIoType;
	WSABUF mWsaBuf;
	char mBuffer[BUF_SIZE];
};

class ClientSession
{
public:
	ClientSession(SOCKET sock) : mConnected(false), mSocket(sock)
	{
		memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));
	}

	~ClientSession() {}

	bool OnConnect(SOCKADDR_IN* addr);
	bool IsConnected() const { return mConnected; }
	bool PostRecv() const;
	bool PostSend(const char* buf, int len) const;
	void Disconnect(DisconnectReason dr);

	bool mConnected;
	SOCKET mSocket;
	SOCKADDR_IN mClientAddr;

private:
	FastSpinlock mLock;
};