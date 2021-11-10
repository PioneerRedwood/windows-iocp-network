#pragma once
#include "predef.hpp"

// 순환 버퍼
class CircularBuffer
{
public:
	// 버퍼 크기로 생성자
	CircularBuffer(size_t capacity)
		: mBRegionPointer{ nullptr }, mARegionSize{ 0 },
		mBRegionSize{0}, mCapacity{capacity}
	{
		mBuffer = new char[capacity];
		mBufferEnd = mBuffer + capacity;
		mARegionPointer = mBuffer;
	}

	~CircularBuffer()
	{
		delete[] mBuffer;
	}

	void BufferReset()
	{
		mBRegionPointer = nullptr;
		mARegionSize = 0;
		mBRegionSize = 0;

		std::memset(mBuffer, 0, mCapacity);

		mBufferEnd = mBuffer + mCapacity;
		mARegionPointer = mBuffer;
	}

	void Remove(size_t len)
	{
		size_t cnt = len;

		if (mARegionSize > 0)
		{
			size_t aRemove = (cnt > mARegionSize) ? mARegionSize : cnt;
			mARegionSize -= aRemove;
			mARegionPointer += aRemove;
			cnt -= aRemove;
		}

		if (cnt > 0 && mBRegionSize > 0)
		{
			size_t bRemove = (cnt > mBRegionSize) ? mBRegionSize : cnt;
			mBRegionSize -= bRemove;
			mBRegionPointer += bRemove;
			cnt -= bRemove;
		}

		if (mARegionSize == 0)
		{
			if (mBRegionSize > 0)
			{
				if (mBRegionPointer != mBuffer)
				{
					memmove(mBuffer, mBRegionPointer, mBRegionSize);
				}

				mARegionPointer = mBuffer;
				mARegionSize = mBRegionSize;
				mBRegionPointer = nullptr;
				mBRegionSize = 0;
			}
			else
			{
				mBRegionPointer = nullptr;
				mBRegionSize = 0;
				mARegionPointer = mBuffer;
				mARegionSize = 0;
			}
		}
	}

	size_t GetFreeSpaceSize()
	{
		if (mBRegionPointer != nullptr)
		{
			return GetBFreeSpace();
		}
		else
		{
			if (GetAFreeSpace() < GetSpaceBeforeA())
			{
				AllocateB();
				return GetBFreeSpace();
			}
			else
			{
				return GetAFreeSpace();
			}
		}
	}

	size_t GetStoredSize() const
	{
		return mARegionSize + mBRegionSize;
	}

	size_t GetContiguiousBytes() const
	{
		if (mARegionSize > 0)
		{
			return mARegionSize;
		}
		else
		{
			return mBRegionSize;
		}
	}

	char* GetBuffer() const
	{
		if (mBRegionPointer != nullptr)
		{
			return mBRegionPointer + mBRegionSize;
		}
		else
		{
			return mARegionPointer + mARegionSize;
		}
	}

	void Commit(size_t len)
	{
		if (mBRegionPointer != nullptr)
		{
			mBRegionSize += len;
		}
		else
		{
			mARegionSize += len;
		}
	}

	// std::STL들은 data()라는 함수가 존재
	char* GetBufferStart() const
	{
		if (mARegionSize > 0)
		{
			return mARegionPointer;
		}
		else
		{
			return mBRegionPointer;
		}
	}
private:
	void AllocateB()
	{
		mBRegionPointer = mBuffer;
	}

	size_t GetAFreeSpace() const
	{
		return (mBufferEnd - mARegionPointer - mARegionSize);
	}

	size_t GetSpaceBeforeA() const
	{
		return (mARegionPointer - mBuffer);
	}

	size_t GetBFreeSpace() const
	{
		if (mBRegionPointer == nullptr)
		{
			return 0;
		}

		return (mARegionPointer - mBRegionPointer - mBRegionSize);
	}

private:
	char* mBuffer;
	char* mBufferEnd;

	char* mARegionPointer;
	size_t mARegionSize;

	char* mBRegionPointer;
	size_t mBRegionSize;

	size_t mCapacity;
};