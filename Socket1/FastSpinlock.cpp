#include "stdafx.h"
#include "FastSpinlock.h"

void FastSpinlock::EnterLock()
{
	for (int nloops = 0; ; nloops++)
	{
		if (InterlockedExchange(&mLockFlag, 1) == 0)
			return;

		UINT uTimerRes = 1;
		timeBeginPeriod(uTimerRes);
		Sleep(static_cast<DWORD>(min(10, nloops)));
		timeEndPeriod(uTimerRes);
	}

}

void FastSpinlock::LeaveLock()
{
	InterlockedExchange(&mLockFlag, 0);
}