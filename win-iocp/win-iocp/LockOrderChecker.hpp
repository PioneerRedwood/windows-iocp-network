#pragma once
#include "predef.hpp"
#include "Exception.hpp"
#include "ThreadLocal.hpp"
#include "FastSpinLock.hpp"

class FastSpinlock;

class LockOrderChecker
{
public:
	LockOrderChecker(int tid)
		: mWorkerThreadId(tid), mStackTopPos(0)
	{
		memset(mLockStack, 0, sizeof(mLockStack));
	}

	void Push(FastSpinlock* lock)
	{
		CRASH_ASSERT(mStackTopPos < MAX_LOCK_DEPTH);

		if (mStackTopPos > 0)
		{
			// 현재 락이 걸려 있는 상태라면 반드시 이전 락의 우선순위가 높아야 함
			// TODO: 그렇지 않으면 CRASH_ASSERT
			//CRASH_ASSERT((int)lock->mLockOrder > )
		}

		mLockStack[mStackTopPos++] = lock;
	}

	void Pop(FastSpinlock* lock)
	{
		// 락이 걸려있는 상태여야 만 함
		CRASH_ASSERT(mStackTopPos > 0);

		// TODO: 최근에 Push했던 녀석과 동일한 지? 아니면 CRASH_ASSERT

		mLockStack[--mStackTopPos] = nullptr;
	}

private:
	enum { MAX_LOCK_DEPTH = 32 };

	int mWorkerThreadId;
	int mStackTopPos;

	FastSpinlock* mLockStack[MAX_LOCK_DEPTH];
};