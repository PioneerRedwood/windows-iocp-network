#pragma once
#include "predef.hpp"
#include "Exception.hpp"
#include "LockOrderChecker.hpp"
#include "ThreadLocal.hpp"

// ���Ǵ� ����
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
		// �� ���� �Ű� �Ƚᵵ �Ǹ� �н�
		if (mLockOrder != (int)LockOrder::LO_DONT_CARE)
		{
			//LLockOrderChecker->Push(this);
		}
		
		while (true)
		{
			// �ٸ����� writelock Ǯ ������ ���
			while (mLockFlag & (int)LockFlag::LF_WRITE_MASK)
			{
				// ???????
				YieldProcessor();
			}

			if ((InterlockedAdd(&mLockFlag, (int)LockFlag::LF_WRITE_FLAG) & (int)LockFlag::LF_WRITE_MASK) == (int)LockFlag::LF_WRITE_FLAG)
			{
				// �ٸ����� readlock Ǯ ������ ���
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
		// �� ���� �Ű� �Ƚᵵ �Ǹ� �н�
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
			// �ٸ����� writelock Ǯ ������ ���
			while (mLockFlag & (int)LockFlag::LF_WRITE_MASK)
			{
				// ???????
				YieldProcessor();
			}

			// TODO: Readlock ���� ���� (mLockFlag�� ��� ó���ϸ� �Ǵ���)
			if (mLockFlag & (int)LockFlag::LF_READ_MASK)
			{
				// return
			}
		}
	}

	void LeaveReadLock()
	{
		// TODO: mLockFlag ó��

		if (mLockOrder != (int)LockOrder::LO_DONT_CARE)
		{
			//LLockOrderChecker->Pop(this);
		}
		else
		{
			// mLockFlag ����
		}
	}

	long GetLockFlag() const { return mLockFlag; }

private:

	enum class LockFlag
	{
		LF_WRITE_MASK = 0x7FF00000,
		LF_WRITE_FLAG = 0x00100000,
		LF_READ_MASK = 0x000FFFFF // ���� 20��Ʈ�� readlock�� ���� �÷��׷� ���
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