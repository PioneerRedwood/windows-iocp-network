#pragma once
#include "Exception.hpp"
#include "TypeTraits.hpp"
#include "XTL.hpp"
#include "ThreadLocal.hpp"
#include <functional>
#include <concurrent_queue.h>

class GrandCentralExecutor
{
public:
	using GCETask = std::function<void()>;

	GrandCentralExecutor()
		: mRemainTaskCount(0) {}

	void DoDispatch(const GCETask& task)
	{
		CRASH_ASSERT(LThreadType == (int)THREAD_TYPE::THREAD_IO_WORKER);

		if (InterlockedIncrement64(&mRemainTaskCount) > 1)
		{
			// TODO: 이미 누가 작업 중이라면?

		}
		else
		{
			// 처음 진입한 녀석이 책임져라
			mCentralTaskQueue.push(task);

			while (true)
			{
				GCETask task;
				if (mCentralTaskQueue.try_pop(task))
				{
					// TODO: task를 수행하고 mRemainTaskCount를 하나 감소
					// mRemainTaskCount가 0이면 break;
					//GCEDispatch()... ?
				}
			}
		}
	}

private:
	using CentralTaskQueue = concurrency::concurrent_queue<GCETask, STLAllocator<GCETask>>;
	CentralTaskQueue mCentralTaskQueue;
	int64_t mRemainTaskCount;
};

template<class T, class F, class... Args>
void GCEDispatch(T instance, F memFunc, Args&&... args)
{
	// shared_ptr가 아닐 경우 받을 수 없음, 작업큐에 있는 동안 사라질 수 있으므로
	static_assert(true == is_shared_ptr<T>::value, "T should be shared_ptr");

	// TODO: instance의 memfunc를 std::bind로 묶어서 전달
	//std::bind(&instance, memFunc, args...);

	//GGrandCentralExecutor->DoDispatch(bind);
}

GrandCentralExecutor* GGrandCentralExecutor = nullptr;