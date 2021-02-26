#include "stdafx.h"
#include "Server.h"
#include "ClientSession.h"
#include "FastSpinlock.h"
#include "SessionManager.h"

#include "Exception.h"

SessionManager* GSessionManager = nullptr;

SessionManager::~SessionManager()
{
	for (auto it : mFreeSessionList) delete it;
}

void SessionManager::PrepareSessions()
{
	CRASH_ASSERT(LThreadType == THREAD_MAIN);
	
	for (int i = 0; i < MAX_CONNECTION; ++i)
	{
		ClientSession* client = new ClientSession();
		mFreeSessionList.push_back(client);
	}
}

void SessionManager::ReturnClientSession(ClientSession* client)
{
	FastSpinlockGuard guard(mLock);
	CRASH_ASSERT(client->mConnected == 0 && client->mRefCount == 0);
	client->SessionReset();
	mFreeSessionList.push_back(client);
	++mCurrentReturnCount;
}

bool SessionManager::AcceptSessions()
{
	FastSpinlockGuard guard(mLock);
	
	while (mCurrentIssueCount - mCurrentReturnCount < MAX_CONNECTION)
	{
		ClientSession* newClient = mFreeSessionList.back();
		mFreeSessionList.pop_back();
		++mCurrentIssueCount;

		newClient->AddRef(); // ref count + 1

		if (false == newClient->PostAccept()) return false;
	}

	return true;
}

