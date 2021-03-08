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
		// TODO ���� ���� �ɸ� ���¿��� �����Ѱ�� �ݵ�� ���� ���� �켱������ ���ƾ� ��. �ƴϸ� CRASH_ASSERT
	}

	mLockStack[mStackTopPos++] = lock;
}

void LockOrderChecker::Pop(FastSpinlock* lock)
{
	// �ּ��� ���� �����ִ� ���¿��� ��
	CRASH_ASSERT(mStackTopPos > 0);

	// TODO : �ֱٿ� push�ߴ��༮�̶� ������ üũ, �ƴϸ� CRASH_ASSERT
	
	mLockStack[--mStackTopPos] = nullptr;
}