#include "Server.h"
#include "stdafx.h"
#include "IocpManager.h"
#include "ClientSession.h"

__declspec(thread) int LThreadType = -1;

int _tmain(int argc, _TCHAR *argv[])
{
	LThreadType = THREAD_MAIN_ACCEPT;
	GIocpManager = new IocpManager;
	if (false == GIocpManager->Initialize()) return -1;
	std::cout << "Start Server\n";
	if (false == GIocpManager->StartAcceptLoop()) return -1;

	GIocpManager->Finalize();
	std::cout << "End Server\n";
	
	delete GIocpManager;

	return 0;
}