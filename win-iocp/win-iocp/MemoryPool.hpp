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
#include "predef.hpp"
#include "Exception.hpp"

// Ŀ�����ϰ� ������ �Ҵ�޴� �ֵ��� ���� �޸� ���� �ٿ��ֱ�
// winnt.h�� SLIST_ENTRY�� ��ӹ������� SList�� ������ �� �ִ� ��Ʈ��(����) ����ü
__declspec(align(MEMORY_ALLOCATION_ALIGNMENT))
struct MemAllocInfo : SLIST_ENTRY
{
	MemAllocInfo(int size) : mAllocSize{size}, mExtraInfo{-1} {}

	// MemAllocInfo�� ���Ե� ũ��
	long mAllocSize;
	// ��Ÿ �߰� ����( Ÿ�� ���� ���� )
	long mExtraInfo;
}; // ��ü 16 ����Ʈ

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

__declspec(align(MEMORY_ALLOCATION_ALIGNMENT))
class SmallSizeMemoryPool
{
public:
	
	SmallSizeMemoryPool(DWORD allocSize)
		: mAllocSize{allocSize}
	{
		CRASH_ASSERT(allocSize > MEMORY_ALLOCATION_ALIGNMENT);
		// SList �ʱ�ȭ, ��� �ּ� �Ѱ��ֱ�
		InitializeSListHead(&mFreeList);
	}

	MemAllocInfo* Pop()
	{

		// TODO: InterlockedPopEntrySList�� �̿��Ͽ� mFreeList���� pop���� �޸𸮸� ������ �� �ְ�
		MemAllocInfo* mem = (MemAllocInfo*)(InterlockedPopEntrySList(&mFreeList));
		if (NULL == mem)
		{
			// �Ҵ� �Ұ����ϸ� ���� �Ҵ�
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
		// TODO: InterlockedPushEntrySList�� �̿��Ͽ� �޸�Ǯ�� ������ ���� �ݳ�
		InterlockedPushEntrySList(&mFreeList, (PSLIST_ENTRY)ptr);
		InterlockedDecrement(&mAllocCount);
	}

private:
	// ù��° ��ġ
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
		// �޸� Ǯ ������ �޸� �ʱ�ȭ
		memset(mSmallSizeMemoryPoolTable, 0, sizeof(mSmallSizeMemoryPoolTable));

		int recent = 0;

		// 0 ~ 1024���� 32���� �޸� �Ҵ�, �� 32���� ������ ������
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

		// TODO: [2048, 4096] ���� �� 256����Ʈ ������ SmallSizeMemoryPool �Ҵ��ϰ�
		// TODO: mSmallSizePoolTable�� O(1) access�� �����ϵ��� SmallSizeMemoryPool�� �ּ� ���
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
			// TODO: SmallSizeMemoryPool ���� �Ҵ�
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
		// ���� �ڿ��� �־ ����ȭ ������ �����ϵ��� �ϴ� ������ ��Ŀ����
		long realAllocSize = InterlockedExchange(&header->mAllocSize, 0);

		CRASH_ASSERT(realAllocSize > 0);

		if (realAllocSize > (int)Config::MAX_ALLOC_SIZE)
		{
			_aligned_free(header);
		}
		else
		{
			// TODO: SmallSizeMemoryPool ������ ���� Push
			mSmallSizeMemoryPoolTable[realAllocSize]->Push(header);
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

	// ���ϴ� ũ���� �޸𸮸� ������ �ִ� Ǯ�� O(1) access�� ���� ���̺�
	SmallSizeMemoryPool* mSmallSizeMemoryPoolTable[(int)Config::MAX_ALLOC_SIZE + 1];
};

extern MemoryPool* GMemoryPool;

// �̰� ����ؾ߸� xnew/xdelete�� �����ϵ���
struct PooledAllocatable {};

template<class T, class... Args>
T* xnew(Args... arg)
{
	static_assert(true == std::is_convertible_v<T, PooledAllocatable>, "Only allowed when PooledAllocatable inherited");

	// TODO: T* obj = xnew<T>(...) ó�� ����� �� �ֵ��� �޸�Ǯ���� �Ҵ��ϰ� ������ �ҷ��ְ� ����
	void* alloc = GMemoryPool->Allocate(sizeof(T));
	new (alloc)T(arg...);
	return reinterpret_cast<T*>(alloc);
}

template<class T>
void xdelete(T* object)
{
	static_assert(true == std::is_convertible_v<T, PooledAllocatable>, "Only allowed when PooledAllocatable inherited");

	// TODO: object�� �Ҹ��� �ҷ��ְ� �޸�Ǯ�� �ݳ�
	object->~T();
	GMemoryPool->Deallocate(object, sizeof(T));
	
}