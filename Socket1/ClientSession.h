#pragma once

// 세션, 각종 이벤트들 -> OnConnect, Disconnect, PostSend, PostRecv 등등

class ClientSession;
class IocpManager;

enum IOType
{
	IO_NONE,
	IO_SEND,
	IO_RECV,
	IO_RECV_ZERO,
	IO_ACCEPT
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
	OverlappedIOContext(const ClientSession* owner, IOType ioType) : mSessionObject(owner), mIoType(ioType)
	{
		memset(&mOverlapped, 0, sizeof(OVERLAPPED));
		memset(&mWsaBuf, 0, sizeof(WSABUF));
		memset(mBuffer, 0, BUF_SIZE);
	}

	OVERLAPPED mOverlapped;
	const ClientSession* mSessionObject;
	IOType mIoType;
	WSABUF mWsaBuf;
	char mBuffer[BUF_SIZE];
};