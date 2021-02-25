#include "stdafx.h"
#include "Server.h"
#include "ClientSession.h"
#include "FastSpinlock.h"
#include "SessionManager.h"

SessionManager* GSessionManager = nullptr;

SessionManager::~SessionManager()
{
	for (auto it : mFreeSessionList) delete it;
}

void SessionManager::PrepareSessions()
{
	for (int i = 0; i < MAX_CONNECTION; ++i)
	{
		ClientSession* client = new ClientSession();
		mFreeSessionList.push_back(client);
	}
}

void SessionManager::ReturnClientSession(ClientSession* client)
{
	FastSpinlockGuard guard(mLock);

	client->SessionReset();
	mFreeSessionList.push_back(client);
	++mCurrentReturnCount;
}

bool SessionManager::AcceptSessions()
{
	FastSpinlockGuard guard(mLock);

	while (mCurrentIssueCount - mCurrentReturnCount < MAX_CONNECTION)
	{
		// TODO : mFreeSessionList에서 ClientSession* 꺼네서 PostAccept() 해주기 (ReturnClientSession과 반대로?)

		// AddRef() 해줘야되고
		// 실패시
		// if (false == newClient->PostAccept()) return false;
	}

	return true;
}

