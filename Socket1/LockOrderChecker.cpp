#include "stdafx.h"
#include "Exception.h"
#include "ThreadLocal.h"
#include "FastSpinlock.h"
#include "LockOrderChecker.h"

LockOrderChecker::LockOrderChecker(int tid) : mWorkerThreadId(tid), mStackTopPos(0)
{
	memset(mLockStack, 0, sizeof(mLockStack));
}

void LockOrderChecker::Push(FastSpinlock* lock)
{
	CRASH_ASSERT(mStackTopPos < MAX_LOCK_DEPTH);

	if (mStackTopPos > 0)
	{
		// TODO 현재 락이 걸린 상태에서 진입한경우 반드시 이전 락의 우선순위가 높아야 함. 아니면 CRASH_ASSERT
	}

	mLockStack[mStackTopPos++] = lock;
}

void LockOrderChecker::Pop(FastSpinlock* lock)
{
	// 최소한 락이 잡혀있는 상태여야 함
	CRASH_ASSERT(mStackTopPos > 0);

	// TODO : 최근에 push했던녀석이랑 같은지 체크, 아니면 CRASH_ASSERT
	
	mLockStack[--mStackTopPos] = nullptr;
}