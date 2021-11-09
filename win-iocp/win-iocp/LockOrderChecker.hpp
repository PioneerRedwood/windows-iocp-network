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
			// ���� ���� �ɷ� �ִ� ���¶�� �ݵ�� ���� ���� �켱������ ���ƾ� ��
			// TODO: �׷��� ������ CRASH_ASSERT
			//CRASH_ASSERT((int)lock->mLockOrder > )
		}

		mLockStack[mStackTopPos++] = lock;
	}

	void Pop(FastSpinlock* lock)
	{
		// ���� �ɷ��ִ� ���¿��� �� ��
		CRASH_ASSERT(mStackTopPos > 0);

		// TODO: �ֱٿ� Push�ߴ� �༮�� ������ ��? �ƴϸ� CRASH_ASSERT

		mLockStack[--mStackTopPos] = nullptr;
	}

private:
	enum { MAX_LOCK_DEPTH = 32 };

	int mWorkerThreadId;
	int mStackTopPos;

	FastSpinlock* mLockStack[MAX_LOCK_DEPTH];
};