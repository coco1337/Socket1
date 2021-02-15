#include "stdafx.h"
#include "ClientSession.h"
#include "FastSpinlock.h"
#include "SessionManager.h"

SessionManager* GSessionManager = nullptr;

ClientSession* SessionManager::CreateClientSession(SOCKET sock)
{
	FastSpinlockGuard guard(mLock);
	ClientSession* client = new ClientSession(sock);

	{
		mClientList.insert(ClientList::value_type(sock, client));
	}

	return client;
}

void SessionManager::DeleteClientSession(ClientSession* client)
{
	FastSpinlockGuard guard(mLock);
	
	{
		mClientList.erase(client->mSocket);
	}

	delete client;
}

