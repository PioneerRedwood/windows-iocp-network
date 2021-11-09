#pragma once
#include "predef.hpp"
#include "Exception.hpp"
#include "LockOrderChecker.hpp"
#include "ThreadLocal.hpp"

// 락되는 순서
enum class LockOrder
{
	LO_DONT_CARE,
	LO_FIRST_CLASS,
	LO_BUSINESS_CLASS,
	LO_ECONOMY_CLASS,
	LO_LUGGAGE_CLASS
};

class LockOrderChecker;

class FastSpinlock
{
public:
	FastSpinlock(const int lockOrder = (int)LockOrder::LO_DONT_CARE)
		: mLockFlag(0), mLockOrder(lockOrder)
	{}

	~FastSpinlock() {}

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

	// exclusive mode
	void EnterWriteLock()
	{
		// 락 순서 신경 안써도 되면 패스
		if (mLockOrder != (int)LockOrder::LO_DONT_CARE)
		{
			//LLockOrderChecker->Push(this);
		}
		
		while (true)
		{
			// 다른놈이 writelock 풀 때까지 대기
			while (mLockFlag & (int)LockFlag::LF_WRITE_MASK)
			{
				// ???????
				YieldProcessor();
			}

			if ((InterlockedAdd(&mLockFlag, (int)LockFlag::LF_WRITE_FLAG) & (int)LockFlag::LF_WRITE_MASK) == (int)LockFlag::LF_WRITE_FLAG)
			{
				// 다른놈이 readlock 풀 때까지 대기
				while (mLockFlag & (int)LockFlag::LF_READ_MASK)
				{
					YieldProcessor();
				}

				return;
			}

			InterlockedAdd(&mLockFlag, -(int)LockFlag::LF_WRITE_FLAG);
		}
	}

	void LeaveWriteLock()
	{
		// 락 순서 신경 안써도 되면 패스
		InterlockedAdd(&mLockFlag, -(int)LockFlag::LF_WRITE_FLAG);

		if (mLockOrder != (int)LockOrder::LO_DONT_CARE)
		{
			//LLockOrderChecker->Pop(this);
		}
	}

	// non-exclusive .. it could be shared
	void EnterReadLock()
	{
		if (mLockOrder != (int)LockOrder::LO_DONT_CARE)
		{
			//LLockOrderChecker->Push(this);
		}

		while (true)
		{
			// 다른놈이 writelock 풀 때까지 대기
			while (mLockFlag & (int)LockFlag::LF_WRITE_MASK)
			{
				// ???????
				YieldProcessor();
			}

			// TODO: Readlock 진입 구현 (mLockFlag를 어떻게 처리하면 되는지)
			if (mLockFlag & (int)LockFlag::LF_READ_MASK)
			{
				// return
			}
		}
	}

	void LeaveReadLock()
	{
		// TODO: mLockFlag 처리

		if (mLockOrder != (int)LockOrder::LO_DONT_CARE)
		{
			//LLockOrderChecker->Pop(this);
		}
		else
		{
			// mLockFlag 원복
		}
	}

	long GetLockFlag() const { return mLockFlag; }

private:

	enum class LockFlag
	{
		LF_WRITE_MASK = 0x7FF00000,
		LF_WRITE_FLAG = 0x00100000,
		LF_READ_MASK = 0x000FFFFF // 하위 20비트를 readlock을 위한 플래그로 사용
	};

	FastSpinlock(const FastSpinlock& rhs);

	volatile long mLockFlag;
	const int mLockOrder;

	friend class LockOrderChecker;
};

class FastSpinlockGuard
{
public:
	FastSpinlockGuard(FastSpinlock& lock, bool exclusive = true)
		: mLock(lock), mExclusiveMode(exclusive)
	{
		if (mExclusiveMode)
		{
			mLock.EnterWriteLock();
		}
		else
		{
			mLock.EnterReadLock();
		}
	}

	~FastSpinlockGuard()
	{
		if (mExclusiveMode)
		{
			mLock.LeaveWriteLock();
		}
		else
		{
			mLock.LeaveReadLock();
		}
	}

private:
	FastSpinlock& mLock;
	bool mExclusiveMode;
};

template<class TargetClass>
class ClassTypeLock
{
public:
	struct LockGuard
	{
		LockGuard()
		{
			TargetClass::mLock.EnterWriteLock();
		}

		~LockGuard()
		{
			TargetClass::mLock.LeaveWriteLock();
		}
	};

private:
	static FastSpinlock mLock;
};

template<class TargetClass>
FastSpinlock ClassTypeLock<TargetClass>::mLock;