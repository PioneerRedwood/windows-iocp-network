#pragma once
#include <functional> // required upper C++17
#include "XTL.hpp"

class SyncExecutable;
using TimerTask = std::function<void()>;
using SyncExecutablePtr = std::shared_ptr<SyncExecutable>;

struct TimerJobElement
{
	TimerJobElement(SyncExecutablePtr owner, const TimerTask& task, int64_t exeTick)
		: mOwner(owner), mTask(task), mExecutionTick(exeTick)
	{}

	virtual ~TimerJobElement() {}

	SyncExecutablePtr mOwner;
	TimerTask mTask;
	int64_t mExecutionTick;
};

struct TimerJobComparator
{
	bool operator()(const TimerJobElement& lhs, const TimerJobElement& rhs)
	{
		return lhs.mExecutionTick > rhs.mExecutionTick;
	}
};

// std::priority_queue
// type, container(vector<T, allocator<T>>), compare func

using TimerJobPriorityQueue =
	std::priority_queue<
		TimerJobElement, 
		std::vector<TimerJobElement, STLAllocator<TimerJobElement>>, TimerJobComparator>;

class Timer
{
public:
	Timer()
	{
		LTickCount = GetTickCount64();
	}

	void PushTimerJob(SyncExecutablePtr owner, const TimerTask& task, uint32_t after)
	{
		CRASH_ASSERT(LThreadType == (int)THREAD_TYPE::THREAD_IO_WORKER);

		// TODO: mTimerJobQueue push
	}

	void DoTimerJob()
	{
		// thread tick update
		LTickCount = GetTickCount64();

		while (!mTimerJobQueue.empty())
		{
			const TimerJobElement& timerJobElem = mTimerJobQueue.top();

			if (LTickCount < timerJobElem.mExecutionTick)
			{
				break;
			}

			//timerJobElem.mOwner->EnterLock();
			//timerJobElem.mTask();
			//timerJobElem.mOwner->LeaveLock();
			//mTimerJobQueue.pop();
		}
	}
private:
	TimerJobPriorityQueue mTimerJobQueue;
};