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

		// TODO: mLock���� ��ȣ�� ���¿��� memfunc�� �����ϰ� ����� R�� ����

	}

	void EnterLock() { mLock.EnterWriteLock(); }
	void LeaveLock() { mLock.LeaveWriteLock(); }

	template<class T>
	std::shared_ptr<T> GetSharedFromThis()
	{
		static_assert(true == std::is_convertible_v<T, SyncExecutable>, "T should be derived from SyncExecutable");

		// TODO: this �����͸� std::shared_ptr<T>���·� ��ȯ
		// static_pointer_cast ���
		return std::static_pointer_cast<std::shared_ptr<T>>(this->shared_from_this());

		//return std::shared_ptr<T>((Player*)this); // �̷��� �ϸ� �ȵɰ�???
	}

private:
	FastSpinlock mLock;
};

template<class T, class F, class... Args>
void DoSyncAfter(uint32_t after, T instance, F memfunc, Args&&... args)
{
	static_assert(true == is_shared_ptr<T>::value, "T should be shared_ptr");
	static_assert(true == std::is_convertible_v<T, SyncExecutable>, "T should be derived from SyncExecutable");

	// TODO: instance�� memfunc�� bind�� ��� LTimer->PushTimerJob() ����

}