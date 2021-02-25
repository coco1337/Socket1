#include "stdafx.h"
#include "Server.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"

__declspec(thread) int LThreadType = -1;

int _tmain(int argc, _TCHAR *argv[])
{
	LThreadType = THREAD_MAIN;
	GSessionManager = new SessionManager;
	GIocpManager = new IocpManager;
	if (false == GIocpManager->Initialize()) return -1;
	if (false == GIocpManager->StartIoThreads()) return -1;
	std::cout << "Start Server\n";
	if (false == GIocpManager->StartAccept()) return -1;
	GIocpManager->Finalize();
	std::cout << "End Server\n";
	
	delete GIocpManager;
	delete GSessionManager;
	return 0;
}