#pragma once
#include "predef.hpp"

class FastSpinlock
{
public:
	FastSpinlock() : mLockFlag(0) {};
	~FastSpinlock() {};

	void EnterLock()
	{
		for (int nloops = 0; ; ++nloops)
		{
			if (InterlockedExchange(&mLockFlag, 1) == 0)
			{
				uint32_t uTimerRes = 1;
				timeBeginPeriod(uTimerRes);
				Sleep((DWORD)min(10, nloops));
				timeEndPeriod(uTimerRes);
			}
		}
	}
	void LeaveLock()
	{
		InterlockedExchange(&mLockFlag, 0);
	}

private:
	FastSpinlock(const FastSpinlock& rhs);
	FastSpinlock& operator=(const FastSpinlock& rhs);

	volatile long mLockFlag;
};

class FastSpinlockGuard
{
public:
	FastSpinlockGuard(FastSpinlock& lock)
		: mLock(lock)
	{
		mLock.EnterLock();
	}

	~FastSpinlockGuard()
	{
		mLock.LeaveLock();
	}

private:
	FastSpinlock& mLock;
};

template<class TargetClass>
class ClassTypeLock
{
public:
	struct LockGuard
	{
		LockGuard()
		{
			TargetClass::mLock.EnterLock();
		}

		~LockGuard()
		{
			TargetClass::mLock.LeaveLock();
		}
	};

private:
	static FastSpinlock mLock;
};

template<class TargetClass>
FastSpinlock ClassTypeLock<TargetClass>::mLock;