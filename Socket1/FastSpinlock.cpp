#include "stdafx.h"
#include "FastSpinlock.h"
#include "LockOrderChecker.h"
#include "ThreadLocal.h"

FastSpinlock::FastSpinlock(const int lockOrder) : mLockFlag(0), mLockOrder(lockOrder) {}
FastSpinlock::~FastSpinlock() {}

void FastSpinlock::EnterWriteLock()
{
	// 락 순서 신경 안써도 되면 패스
	if (mLockOrder != LO_DONT_CARE) LLockOrderChecker->Push(this);
	while (true)
	{
		// 다른놈이 writelock 풀어줄때까지 기다림
		while (mLockFlag & LF_WRITE_MASK)	YieldProcessor();
		if ((InterlockedAdd(&mLockFlag, LF_WRITE_FLAG) & LF_WRITE_MASK) == LF_WRITE_FLAG)
		{
			// 다른놈이 readlock 풀어줄때까지 기다린다
			while (mLockFlag & LF_READ_MASK)	YieldProcessor();
			return;
		}

		InterlockedAdd(&mLockFlag, -LF_WRITE_FLAG);
	}
}

void FastSpinlock::LeaveWriteLock()
{
	InterlockedAdd(&mLockFlag, -LF_WRITE_FLAG);

	// 락 순서 안써도 되면 패스
	if (mLockOrder != LO_DONT_CARE) LLockOrderChecker->Pop(this);
}

void FastSpinlock::EnterReadLock()
{
	if (mLockOrder != LO_DONT_CARE) LLockOrderChecker->Push(this);

	while(true)
	{
		// 다른놈이 writelock 풀어줄때까지 기다린다
		while (mLockFlag & LF_WRITE_MASK) YieldProcessor();

		// TODO Readlock 진입 구현(mLockFlag 처리)
		// if (readlock 얻으면) return;
		// else mLockFlag 원상복구
	}
}

