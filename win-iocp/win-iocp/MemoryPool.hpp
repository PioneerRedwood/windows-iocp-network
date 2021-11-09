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
#include "predef.hpp"
#include "Exception.hpp"

// 커스텀하게 힙에서 할당받는 애들은 전부 메모리 정보 붙여주기
// winnt.h의 SLIST_ENTRY를 상속받음으로 SList로 관리될 수 있는 엔트리(형식) 구조체
__declspec(align(MEMORY_ALLOCATION_ALIGNMENT))
struct MemAllocInfo : SLIST_ENTRY
{
	MemAllocInfo(int size) : mAllocSize{size}, mExtraInfo{-1} {}

	// MemAllocInfo가 포함된 크기
	long mAllocSize;
	// 기타 추가 정보( 타입 관련 정보 )
	long mExtraInfo;
}; // 전체 16 바이트

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

__declspec(align(MEMORY_ALLOCATION_ALIGNMENT))
class SmallSizeMemoryPool
{
public:
	
	SmallSizeMemoryPool(DWORD allocSize)
		: mAllocSize{allocSize}
	{
		CRASH_ASSERT(allocSize > MEMORY_ALLOCATION_ALIGNMENT);
		// SList 초기화, 헤더 주소 넘겨주기
		InitializeSListHead(&mFreeList);
	}

	MemAllocInfo* Pop()
	{

		// TODO: InterlockedPopEntrySList를 이용하여 mFreeList에서 pop으로 메모리를 가져올 수 있게
		MemAllocInfo* mem = (MemAllocInfo*)(InterlockedPopEntrySList(&mFreeList));
		if (NULL == mem)
		{
			// 할당 불가능하면 직접 할당
			mem = reinterpret_cast<MemAllocInfo*>(_aligned_malloc(mAllocSize, MEMORY_ALLOCATION_ALIGNMENT));
		}
		else
		{
			CRASH_ASSERT(mem->mAllocSize == 0);
		}

		InterlockedIncrement(&mAllocCount);
		return mem;

	}
	void Push(MemAllocInfo* ptr)
	{
		// TODO: InterlockedPushEntrySList를 이용하여 메모리풀에 재사용을 위해 반납
		InterlockedPushEntrySList(&mFreeList, (PSLIST_ENTRY)ptr);
		InterlockedDecrement(&mAllocCount);
	}

private:
	// 첫번째 위치
	SLIST_HEADER mFreeList;

	const DWORD mAllocSize;
	volatile long mAllocCount = 0;
};

////////////////////////////////////////////////////////////////////////////////

class MemoryPool
{
public:
	MemoryPool()
	{
		// 메모리 풀 사이즈 메모리 초기화
		memset(mSmallSizeMemoryPoolTable, 0, sizeof(mSmallSizeMemoryPoolTable));

		int recent = 0;

		// 0 ~ 1024까지 32구역 메모리 할당, 총 32개의 구역이 생성됨
		for (int i = 32; i < 1024; i += 32)
		{
			SmallSizeMemoryPool* pool = new SmallSizeMemoryPool(i);
			for (int j = recent + 1; j <= i; ++j)
			{
				mSmallSizeMemoryPoolTable[j] = pool;
			}
			recent = i;
		}

		for (int i = 1024; i < 2048; i += 128)
		{
			SmallSizeMemoryPool* pool = new SmallSizeMemoryPool(i);
			for (int j = recent + 1; j <= i; ++j)
			{
				mSmallSizeMemoryPoolTable[j] = pool;
			}
			recent = i;
		}

		// TODO: [2048, 4096] 범위 내 256바이트 단위로 SmallSizeMemoryPool 할당하고
		// TODO: mSmallSizePoolTable에 O(1) access가 가능하도록 SmallSizeMemoryPool의 주소 기록
		for (int i = 2048; i < 4096; i += 256)
		{
			SmallSizeMemoryPool* pool = new SmallSizeMemoryPool(i);
			for (int j = recent + 1; j <= i; ++j)
			{
				mSmallSizeMemoryPoolTable[j] = pool;
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
			// TODO: SmallSizeMemoryPool 에서 할당
			header = mSmallSizeMemoryPoolTable[realAllocSize]->Pop();
		}

		return AttachMemAllocInfo(header, realAllocSize);
	}

	void Deallocate(void* ptr, long extraInfo)
	{
		MemAllocInfo* header = DetachMemAllocInfo(ptr);
		header->mExtraInfo = extraInfo;

		// The interlocked functions provide a simple mechanism 
		// for synchronizing access to a variable 
		// that is shared by multiple threads. 
		// This function is atomic with respect to calls to other interlocked functions.
		// 공유 자원에 있어서 동기화 접근이 가능하도록 하는 간단한 메커니즘
		long realAllocSize = InterlockedExchange(&header->mAllocSize, 0);

		CRASH_ASSERT(realAllocSize > 0);

		if (realAllocSize > (int)Config::MAX_ALLOC_SIZE)
		{
			_aligned_free(header);
		}
		else
		{
			// TODO: SmallSizeMemoryPool 재사용을 위해 Push
			mSmallSizeMemoryPoolTable[realAllocSize]->Push(header);
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

	// 원하는 크기의 메모리를 가지고 있는 풀에 O(1) access를 위한 테이블
	SmallSizeMemoryPool* mSmallSizeMemoryPoolTable[(int)Config::MAX_ALLOC_SIZE + 1];
};

extern MemoryPool* GMemoryPool;

// 이걸 상속해야만 xnew/xdelete이 가능하도록
struct PooledAllocatable {};

template<class T, class... Args>
T* xnew(Args... arg)
{
	static_assert(true == std::is_convertible_v<T, PooledAllocatable>, "Only allowed when PooledAllocatable inherited");

	// TODO: T* obj = xnew<T>(...) 처럼 사용할 수 있도록 메모리풀에서 할당하고 생성자 불러주고 리턴
	void* alloc = GMemoryPool->Allocate(sizeof(T));
	new (alloc)T(arg...);
	return reinterpret_cast<T*>(alloc);
}

template<class T>
void xdelete(T* object)
{
	static_assert(true == std::is_convertible_v<T, PooledAllocatable>, "Only allowed when PooledAllocatable inherited");

	// TODO: object의 소멸자 불러주고 메모리풀에 반납
	object->~T();
	GMemoryPool->Deallocate(object, sizeof(T));
	
}