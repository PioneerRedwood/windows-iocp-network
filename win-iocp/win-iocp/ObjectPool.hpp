#pragma once
#include "predef.hpp"
#include "Exception.hpp"

template<class Target>
class ClassLockGuard
{
public:
	struct LockGuard
	{
		LockGuard()
		{
			std::lock_guard<std::mutex> lock(mMutex);
		}

		~LockGuard()
		{

		}
	};

private:
	std::mutex mMutex;
};

template<class Container, int ALLOC_COUNT = 100>
class ObjectPool
{
public:
	static void* operator new(size_t objSize)
	{
		// TODO: Container 타입 단위로 lock 잠금
		std::lock_guard<std::mutex> lock(mMutex);
		if (!mFreeList)
		{
			mFreeList = new uint8_t[sizeof(Container) * ALLOC_COUNT];

			uint8_t* pNext = mFreeList;
			uint8_t** ppCurr = reinterpret_cast<uint8_t**>(mFreeList);

			for (int i = 0; i < ALLOC_COUNT - 1; ++i)
			{
				pNext += sizeof(Container);
				*ppCurr = pNext;
				ppCurr = reinterpret_cast<uint8_t**>(pNext);
			}

			mTotalAllocCount += ALLOC_COUNT;
		}

		uint8_t* pAvaliable = mFreeList;
		mFreeList = *reinterpret_cast<uint8_t**>(pAvailable);
		++mCurrentUseCount;

		return pAvaliable;
	}

	static void operator delete(void* obj)
	{
		// TODO: Container 타입 단위로 lock 잠금
		std::lock_guard<std::mutex> lock(mMutex);
		CRASH_ASSERT(mCurrentUseCount > 0);

		--mCurrentUseCount;

		*reinterpret_cast<uint8_t**>(obj) = mFreeList;
		mFreeList = static_cast<uint8_t*>(obj);
	}

private:
	static uint8_t* mFreeList;
	static int mTotalAllocCount;
	static int mCurrentUseCount;
	static std::mutex mMutex;
};

// static member variables intialize
template<class Container, int ALLOC_COUNT>
uint8_t* ObjectPool<Container, ALLOC_COUNT>::mFreeList = nullptr;

template<class Container, int ALLOC_COUNT>
int ObjectPool<Container, ALLOC_COUNT>::mTotalAllocCount = 0;

template<class Container, int ALLOC_COUNT>
int ObjectPool<Container, ALLOC_COUNT>::mCurrentUseCount = 0;
