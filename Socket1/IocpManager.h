#pragma once

class ClientSession;

struct OverlappedSendContext;
struct OverlappedPreRecvContext;
struct OverlappedRecvContext;
struct OverlappedDisconnectContext;
struct OverlappedAcceptContext;

// iocp 연결과 관련된 함수들

class IocpManager
{
public:
	IocpManager();
	~IocpManager();
	
	bool Initialize();
	void Finalize();

	bool StartIoThreads();
	bool StartAccept();

	HANDLE GetCompletionPort() { return mCompletionPort; }
	int GetIoThreadCount() { return mIoThreadCount; }

	SOCKET* GetListenSocket() { return &mListenSocket; }

	static char mAcceptBuf[64];
	static LPFN_DISCONNECTEX mFnDisconnectEx;
	static LPFN_ACCEPTEX mFnAcceptEx;

private:
	static unsigned int WINAPI IoWorkerThread(LPVOID lpParam);

	static bool PreReceiveCompletion(ClientSession* client, OverlappedPreRecvContext* context, DWORD dwTransferred);
	static bool ReceiveCompletion(ClientSession* client, OverlappedRecvContext* context, DWORD dwTransferred);
	static bool SendCompletion(ClientSession* client, OverlappedSendContext* context, DWORD dwTransferred);
	
	
	int mIoThreadCount;
	HANDLE mCompletionPort;
	SOCKET mListenSocket;
};

extern __declspec(thread) int LIoThreadId;
extern IocpManager* GIocpManager;