#pragma once
#include "predef.hpp"
#include "CircularBuffer.hpp"
#include "OverlappedIOContext.hpp"
#include "FastSpinLock.hpp"
#include "Exception.hpp"
#include "ThreadLocal.hpp"
#include "IocpManager.hpp"

class Session
{
public:
	Session(size_t sendBufSize, size_t recvBufSize)
		: mSendBuffer(sendBufSize), mRecvBuffer(recvBufSize), mConnected(0), mRefCount(0), mSendPendingCount(0)
	{
		mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	}

	virtual ~Session() {}

	bool IsConnected() const { return !!mConnected; }

	void DisconnectRequest(DisconnectReason dr)
	{
		if (0 == InterlockedExchange(&mConnected, 0))
		{
			return;
		}

		OverlappedDisconnectContext* context = new OverlappedDisconnectContext(this, dr);

		// Disconnectex의 정의를 찾을 수 없다는 이상한 오류로
		// 대체 함수를 넣어둠
		if (FALSE == (LPFN_DISCONNECTEX)(mSocket, (LPOVERLAPPED)context, TF_REUSE_SOCKET, 0))
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				DeleteIoContext(context);
				// 로그 남기기
			}
		}
	}

	bool PreRecv()
	{
		if (!IsConnected())
		{
			return false;
		}

		OverlappedPreRecvContext* context = new OverlappedPreRecvContext(this);

		DWORD recvBytes = 0;
		DWORD flags = 0;
		context->mWsaBuf.len = 0;
		context->mWsaBuf.buf = nullptr;

		// start async recv
		if (SOCKET_ERROR == WSARecv(mSocket, &context->mWsaBuf, 1, &recvBytes, &flags, (LPWSAOVERLAPPED)context, NULL))
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				DeleteIoContext(context);
				// 로그 남기기
				return false;
			}
		}

		return true;
	}

	bool PostRecv()
	{
		if (!IsConnected())
		{
			return false;
		}

		if (0 == mRecvBuffer.GetFreeSpaceSize())
		{
			return false;
		}

		OverlappedRecvContext* context = new OverlappedRecvContext(this);

		DWORD recvBytes = 0;
		DWORD flags = 0;
		context->mWsaBuf.len = 0;
		context->mWsaBuf.buf = nullptr;

		// start async recv
		if (SOCKET_ERROR == WSARecv(mSocket, &context->mWsaBuf, 1, &recvBytes, &flags, (LPWSAOVERLAPPED)context, NULL))
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				DeleteIoContext(context);
				// 로그 남기기
				return false;
			}
		}

		return true;
	}

	bool PostSend(const char* data, size_t len)
	{
		if (!IsConnected())
		{
			return false;
		}

		FastSpinlockGuard criticalSection(mSendBufferLock);

		if (mSendBuffer.GetFreeSpaceSize() < len)
		{
			return false;
		}

		// flush later
		//LSendRequestSessionList->push_back(this);
		char* destData = mSendBuffer.GetBuffer();
		memcpy(destData, data, len);
		mSendBuffer.Commit(len);

		return true;
	}

	bool FlushSend()
	{
		if (!IsConnected())
		{
			DisconnectRequest(DisconnectReason::DR_SENDFLUSH_ERROR);
			return true;
		}

		FastSpinlockGuard criticalSection(mSendBufferLock);

		// 보낼 데이터가 없는 경우
		if (0 == mSendBuffer.GetContiguiousBytes()) 
		{
			// 보낼 데이터도 없는 경우
			if (0 == mSendPendingCount)
			{
				return true;
			}

			return false;
		}

		if (mSendPendingCount > 0)
		{
			return false;
		}

		OverlappedSendContext* context = new OverlappedSendContext(this);

		DWORD sendBytes = 0;
		DWORD flags = 0;
		context->mWsaBuf.len = (ULONG)mSendBuffer.GetContiguiousBytes();
		context->mWsaBuf.buf = mSendBuffer.GetBufferStart();

		if (SOCKET_ERROR == WSASend(mSocket, &context->mWsaBuf, 1, &sendBytes, &flags, (LPWSAOVERLAPPED)context, NULL))
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				DeleteIoContext(context);
				// error log

				DisconnectRequest(DisconnectReason::DR_SENDFLUSH_ERROR);
				return true;
			}
		}

		mSendPendingCount++;

		return mSendPendingCount == 1;
	}

	void DisconnectCompletion(DisconnectReason dr)
	{
		OnDisconnect(dr);
		ReleaseRef();
	}

	void SendCompletion(DWORD transferred)
	{
		FastSpinlockGuard criticalSection(mSendBufferLock);
		mSendBuffer.Remove(transferred);
		mSendPendingCount--;
	}

	void RecvCompletion(DWORD transferred)
	{
		mRecvBuffer.Commit(transferred);
	}

	void AddRef()
	{
		CRASH_ASSERT(InterlockedIncrement(&mRefCount) > 0);
	}

	void ReleaseRef()
	{
		long ret = InterlockedDecrement(&mRefCount);
		CRASH_ASSERT(ret >= 0);

		if (ret == 0)
		{
			OnRelease();
		}
	}

	virtual void OnDisconnect(DisconnectReason dr) {}
	virtual void OnRelease() {}

	void SetSocket(SOCKET sock) { mSocket = sock; }
	SOCKET GetSocket() const { return mSocket; }

	void EchoBack()
	{
		size_t len = mRecvBuffer.GetContiguiousBytes();

		if (len == 0)
		{
			return;
		}

		if (false == PostSend(mRecvBuffer.GetBufferStart(), len))
		{
			return;
		}

		mRecvBuffer.Remove(len);
	}

protected:
	SOCKET mSocket;
	CircularBuffer mRecvBuffer;
	CircularBuffer mSendBuffer;
	FastSpinlock mSendBufferLock;
	int mSendPendingCount;

	volatile long mRefCount;
	volatile long mConnected;

};