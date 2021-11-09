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
			// TODO: �̹� ���� �۾� ���̶��?

		}
		else
		{
			// ó�� ������ �༮�� å������
			mCentralTaskQueue.push(task);

			while (true)
			{
				GCETask task;
				if (mCentralTaskQueue.try_pop(task))
				{
					// TODO: task�� �����ϰ� mRemainTaskCount�� �ϳ� ����
					// mRemainTaskCount�� 0�̸� break;
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
	// shared_ptr�� �ƴ� ��� ���� �� ����, �۾�ť�� �ִ� ���� ����� �� �����Ƿ�
	static_assert(true == is_shared_ptr<T>::value, "T should be shared_ptr");

	// TODO: instance�� memfunc�� std::bind�� ��� ����
	//std::bind(&instance, memFunc, args...);

	//GGrandCentralExecutor->DoDispatch(bind);
}

GrandCentralExecutor* GGrandCentralExecutor = nullptr;