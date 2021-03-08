#pragma once
#include "MemoryPool.h"
#include "ObjectPool.h"
#include "CircularBuffer.h"
#include "FastSpinlock.h"

// 세션, 각종 이벤트들 -> OnConnect, Disconnect, PostSend, PostRecv 등등

class ClientSession;
class SessionManager;

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
	DR_ACTIVE,
	DR_ONCONNECT_ERROR,
	DR_IO_REQUEST_ERROR,
	DR_COMPLETION_ERROR,
};

struct OverlappedIOContext
{
	OverlappedIOContext(ClientSession* owner, IOType ioType);

	OVERLAPPED mOverlapped;
	ClientSession* mSessionObject;
	IOType mIoType;
	WSABUF mWsaBuf;
};

struct OverlappedSendContext : public OverlappedIOContext, public ObjectPool<OverlappedSendContext>
{
	OverlappedSendContext(ClientSession* owner) : OverlappedIOContext(owner, IO_SEND) {}
};

struct OverlappedRecvContext : public OverlappedIOContext, public ObjectPool<OverlappedRecvContext>
{
	OverlappedRecvContext(ClientSession* owner) : OverlappedIOContext(owner, IO_RECV) {}
};

struct OverlappedPreRecvContext : public OverlappedIOContext, public ObjectPool<OverlappedPreRecvContext>
{
	OverlappedPreRecvContext(ClientSession* owner) : OverlappedIOContext(owner, IO_RECV_ZERO) {}
};

struct OverlappedDisconnectContext : public OverlappedIOContext, public ObjectPool<OverlappedIOContext>
{
	OverlappedDisconnectContext(ClientSession* owner, DisconnectReason dr)
		: OverlappedIOContext(owner, IO_DISCONNECT), mDisconnectReason(dr) {}

	DisconnectReason mDisconnectReason;
};

struct OverlappedAcceptContext : public OverlappedIOContext
{
	OverlappedAcceptContext(ClientSession* owner) :OverlappedIOContext(owner, IO_ACCEPT) {}
};

void DeleteIoContext(OverlappedIOContext* context);

class ClientSession : public PooledAllocatable
{
public:
	ClientSession();
	~ClientSession() {}

	void SessionReset();
	bool IsConnected() const { return !!mConnected; }

	bool PostAccept();
	void AcceptCompletion();

	bool PreRecv(); // zero byte recv

	bool PostRecv();
	void RecvCompletion(DWORD transferred);

	bool PostSend();
	void SendCompletion(DWORD transferred);

	void DisconnectRequest(DisconnectReason dr);
	void DisconnectCompletion(DisconnectReason dr);

	void AddRef();
	void ReleaseRef();

	void SetSocket(SOCKET sock) { mSocket = sock; }
	SOCKET GetSocket() const { return mSocket; }

private:
	SOCKET mSocket;
	SOCKADDR_IN mClientAddr;
	
	FastSpinlock mBufferLock;

	CircularBuffer mBuffer;

	volatile long mRefCount;
	volatile long mConnected;

	friend class SessionManager;
};