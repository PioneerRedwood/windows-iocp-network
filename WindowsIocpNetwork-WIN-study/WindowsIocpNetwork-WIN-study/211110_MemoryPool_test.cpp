/* 학습자 코멘트

메모리의 잦은 할당과 파괴 등의 시스템 호출은 성능에 영향을 미치기 때문에
최대한 동적 할당과 파괴를 줄일 필요가 있다.

메모리 관리를 효율적으로 하기 위해 winnt.h에 선언돼있는 Interlocked Singly Linked Lists를 사용

An interlocked singly linked list (SList) eases the task of insertion and deletion
from a linked list. SLists are implemented using a nonblocking algorithm
to provide atomic synchronization, increase system performance,
and avoid problems such as priority inversion and lock convoys.

연결 리스트로부터 삽입과 삭제를 용이하게 함.
원자단위의 동기화, 시스템 퍼포먼스 상승, 우선순위 반전, 잠금 보호(convoys?)를 위해
논블로킹 알고리즘으로 구현돼있다.

멀티 스레드에서 사용될 수 있음

내부적으로 Stack의 형태로 진행 Push & Pop
*/

#pragma once
#include <sdkddkver.h>

#include <iostream>
#include <memory>
#include <mutex>

#include <WinSock2.h>
#include <Mswsock.h>

#include <stdio.h>
#include <tchar.h>
#include <process.h>
#include <assert.h>
#include <limits.h>

#include <cstdint>

inline void __ASSERT(bool isOk)
{
	if (isOk) return;

	int* crashVal = 0;
	*crashVal = 0xDEADBEF;
}

class MemoryPool
{
public:
	struct Poollable {};
private:
	// winnt.h의 SLIST_ENTRY를 상속받음으로 SList로 관리될 수 있는 엔트리(형식) 구조체
	__declspec(align(MEMORY_ALLOCATION_ALIGNMENT))
		struct MemAllocInfo : SLIST_ENTRY
	{
		MemAllocInfo(int size) : mAllocSize{ size }, mExtraInfo{ -1 } {}

		long mAllocSize;
		long mExtraInfo;
	};

	inline void* AttachMemAllocInfo(MemAllocInfo* header, int size)
	{
		// TODO: header에 MemAllocInfo를 펼친 다음에 실제 앱에서 사용할 메모리 주소를 void*로 리턴
		// 실제 사용되는 예 및 DetachMemAllocInfo 참고
		new (header)MemAllocInfo(size);
		return reinterpret_cast<void*>(++header);
	}

	inline MemAllocInfo* DetachMemAllocInfo(void* ptr)
	{
		MemAllocInfo* header = reinterpret_cast<MemAllocInfo*>(ptr);
		--header;
		return header;
	}


private:
	__declspec(align(MEMORY_ALLOCATION_ALIGNMENT))
		class SmallSizeMemoryPool
	{
	public:

		SmallSizeMemoryPool(DWORD allocSize)
			: mAllocSize{ allocSize }
		{
			__ASSERT(allocSize > MEMORY_ALLOCATION_ALIGNMENT);
			InitializeSListHead(&mFreeList);
		}

		MemAllocInfo* Pop()
		{
			MemAllocInfo* mem = (MemAllocInfo*)(InterlockedPopEntrySList(&mFreeList));
			if (NULL == mem)
			{
				// 할당 불가능하면 직접 할당
				mem = reinterpret_cast<MemAllocInfo*>(_aligned_malloc(mAllocSize, MEMORY_ALLOCATION_ALIGNMENT));
			}
			else
			{
				__ASSERT(mem->mAllocSize == 0);
			}

			InterlockedIncrement(&mAllocCount);
			return mem;

		}
		void Push(MemAllocInfo* ptr)
		{
			InterlockedPushEntrySList(&mFreeList, (PSLIST_ENTRY)ptr);
			InterlockedDecrement(&mAllocCount);
		}

	private:
		// 첫번째 위치
		SLIST_HEADER mFreeList;

		const DWORD mAllocSize;
		volatile long mAllocCount = 0;
	};

public:
	MemoryPool()
	{
		memset(memoryPoolTable, 0, sizeof(memoryPoolTable));

		int recent = 0;
		for (int i = 32; i < 1024; i += 32)
		{
			SmallSizeMemoryPool* pool = new SmallSizeMemoryPool(i);
			for (int j = recent + 1; j <= i; ++j)
			{
				memoryPoolTable[j] = pool;
			}
			recent = i;
		}

		for (int i = 1024; i < 2048; i += 128)
		{
			SmallSizeMemoryPool* pool = new SmallSizeMemoryPool(i);
			for (int j = recent + 1; j <= i; ++j)
			{
				memoryPoolTable[j] = pool;
			}
			recent = i;
		}

		for (int i = 2048; i < 4096; i += 256)
		{
			SmallSizeMemoryPool* pool = new SmallSizeMemoryPool(i);
			for (int j = recent + 1; j <= i; ++j)
			{
				memoryPoolTable[j] = pool;
			}
			recent = i;
		}
	}

	void* Allocate(int size)
	{
		MemAllocInfo* header = nullptr;
		int realAllocSize = size + sizeof(MemAllocInfo);

		if (realAllocSize > (int)Config::MAX_ALLOC_SIZE)
		{
			header = reinterpret_cast<MemAllocInfo*>(_aligned_malloc(realAllocSize, MEMORY_ALLOCATION_ALIGNMENT));
		}
		else
		{
			header = memoryPoolTable[realAllocSize]->Pop();
		}

		return AttachMemAllocInfo(header, realAllocSize);
	}

	void Deallocate(void* ptr, long extraInfo)
	{
		MemAllocInfo* header = DetachMemAllocInfo(ptr);
		header->mExtraInfo = extraInfo;

		long realAllocSize = InterlockedExchange(&header->mAllocSize, 0);

		__ASSERT(realAllocSize > 0);

		if (realAllocSize > (int)Config::MAX_ALLOC_SIZE)
		{
			_aligned_free(header);
		}
		else
		{
			memoryPoolTable[realAllocSize]->Push(header);
		}
	}

private:
	enum class Config
	{
		// 함부로 변경하면 안됨. 철저히 계산하고 변경할 것.
		// ~1024까지 32단위, ~2048까지 128단위, ~4096까지 256단위
		MAX_SMALL_POOL_COUNT = 1024 / 32 + 1024 / 128 + 2048 / 256,
		MAX_ALLOC_SIZE = 4096
	};

	SmallSizeMemoryPool* memoryPoolTable[(int)Config::MAX_ALLOC_SIZE + 1];
};

// Global
MemoryPool* GMemoryPool = nullptr;

template<class T, class... Args>
T* xnew(Args... arg)
{
	static_assert(true == std::is_convertible_v<T, MemoryPool::Poollable>, "Only allowed when Poollable inherited");

	// TODO: T* obj = xnew<T>(...) 처럼 사용할 수 있도록 메모리풀에서 할당하고 생성자 불러주고 리턴
	void* alloc = GMemoryPool->Allocate(sizeof(T));
	new (alloc)T(arg...);
	return reinterpret_cast<T*>(alloc);
} 

template<class T>
void xdelete(T* object)
{
	static_assert(true == std::is_convertible_v<T, MemoryPool::Poollable>, "Only allowed when Poollable inherited");

	// TODO: object의 소멸자 불러주고 메모리풀에 반납
	object->~T();
	GMemoryPool->Deallocate(object, sizeof(T));
}

int main()
{
	GMemoryPool = new MemoryPool();

	struct TestObject1 : public MemoryPool::Poollable { TestObject1(uint32_t id) : MemoryPool::Poollable(), id(id) {} uint32_t id; };
	 
	struct TestObject2 {};

	TestObject1* to1 = xnew<TestObject1>((uint32_t)1);
	xdelete<TestObject1>(to1);

	// new 연산자가 오른쪽에 있지 않은 것은 처음 봐서 뇌에 혼란이 왔다.
	// 게다가 형식 지정자 앞에 캐스트가 있는 것은 대체 무슨 역할을 하는지 감이 안왔다.
	// 뭐랄까 소유권을 넘기는 개념이라고 해야하나
	new (&to1)TestObject1{(uint32_t)0};

	

	return 0;
}