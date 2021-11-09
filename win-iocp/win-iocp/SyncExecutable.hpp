#pragma once
#include "TypeTraits.hpp"
#include "FastSpinLock.hpp"
#include "Timer.hpp"

class SyncExecutable : public std::enable_shared_from_this<SyncExecutable>
{
public:
	SyncExecutable()
		: mLock((int)LockOrder::LO_BUSINESS_CLASS)
	{}

	~SyncExecutable() {}

	template<class R, class T, class... Args>
	R DoSync(R(T::* memfunc)(Args...), Args... args)
	{
		static_assert(true == std::is_convertible_v<T, SyncExecutable>, "T should be derived from SyncExecutable");

		// TODO: mLock으로 보호한 상태에서 memfunc를 실행하고 결과값 R을 리턴

	}

	void EnterLock() { mLock.EnterWriteLock(); }
	void LeaveLock() { mLock.LeaveWriteLock(); }

	template<class T>
	std::shared_ptr<T> GetSharedFromThis()
	{
		static_assert(true == std::is_convertible_v<T, SyncExecutable>, "T should be derived from SyncExecutable");

		// TODO: this 포인터를 std::shared_ptr<T>형태로 반환
		// static_pointer_cast 사용
		return std::static_pointer_cast<std::shared_ptr<T>>(this->shared_from_this());

		//return std::shared_ptr<T>((Player*)this); // 이렇게 하면 안될걸???
	}

private:
	FastSpinlock mLock;
};

template<class T, class F, class... Args>
void DoSyncAfter(uint32_t after, T instance, F memfunc, Args&&... args)
{
	static_assert(true == is_shared_ptr<T>::value, "T should be shared_ptr");
	static_assert(true == std::is_convertible_v<T, SyncExecutable>, "T should be derived from SyncExecutable");

	// TODO: instance의 memfunc을 bind로 묶어서 LTimer->PushTimerJob() 수행

}