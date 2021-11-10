/* �н��� �ڸ�Ʈ

�޸��� ���� �Ҵ�� �ı� ���� �ý��� ȣ���� ���ɿ� ������ ��ġ�� ������
�ִ��� ���� �Ҵ�� �ı��� ���� �ʿ䰡 �ִ�.

�޸� ������ ȿ�������� �ϱ� ���� winnt.h�� ������ִ� Interlocked Singly Linked Lists�� ���

An interlocked singly linked list (SList) eases the task of insertion and deletion
from a linked list. SLists are implemented using a nonblocking algorithm
to provide atomic synchronization, increase system performance,
and avoid problems such as priority inversion and lock convoys.

���� ����Ʈ�κ��� ���԰� ������ �����ϰ� ��.
���ڴ����� ����ȭ, �ý��� �����ս� ���, �켱���� ����, ��� ��ȣ(convoys?)�� ����
����ŷ �˰������� �������ִ�.

��Ƽ �����忡�� ���� �� ����

���������� Stack�� ���·� ���� Push & Pop
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
	// winnt.h�� SLIST_ENTRY�� ��ӹ������� SList�� ������ �� �ִ� ��Ʈ��(����) ����ü
	__declspec(align(MEMORY_ALLOCATION_ALIGNMENT))
		struct MemAllocInfo : SLIST_ENTRY
	{
		MemAllocInfo(int size) : mAllocSize{ size }, mExtraInfo{ -1 } {}

		long mAllocSize;
		long mExtraInfo;
	};

	inline void* AttachMemAllocInfo(MemAllocInfo* header, int size)
	{
		// TODO: header�� MemAllocInfo�� ��ģ ������ ���� �ۿ��� ����� �޸� �ּҸ� void*�� ����
		// ���� ���Ǵ� �� �� DetachMemAllocInfo ����
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
				// �Ҵ� �Ұ����ϸ� ���� �Ҵ�
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
		// ù��° ��ġ
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
		// �Ժη� �����ϸ� �ȵ�. ö���� ����ϰ� ������ ��.
		// ~1024���� 32����, ~2048���� 128����, ~4096���� 256����
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

	// TODO: T* obj = xnew<T>(...) ó�� ����� �� �ֵ��� �޸�Ǯ���� �Ҵ��ϰ� ������ �ҷ��ְ� ����
	void* alloc = GMemoryPool->Allocate(sizeof(T));
	new (alloc)T(arg...);
	return reinterpret_cast<T*>(alloc);
} 

template<class T>
void xdelete(T* object)
{
	static_assert(true == std::is_convertible_v<T, MemoryPool::Poollable>, "Only allowed when Poollable inherited");

	// TODO: object�� �Ҹ��� �ҷ��ְ� �޸�Ǯ�� �ݳ�
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

	// new �����ڰ� �����ʿ� ���� ���� ���� ó�� ���� ���� ȥ���� �Դ�.
	// �Դٰ� ���� ������ �տ� ĳ��Ʈ�� �ִ� ���� ��ü ���� ������ �ϴ��� ���� �ȿԴ�.
	// ������ �������� �ѱ�� �����̶�� �ؾ��ϳ�
	new (&to1)TestObject1{(uint32_t)0};

	

	return 0;
}