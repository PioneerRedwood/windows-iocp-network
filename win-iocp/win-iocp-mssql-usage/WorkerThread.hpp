#pragma once
#include "predef.hpp"
#include "OverlappedIOContext.hpp"
#include "Session.hpp"
#include "ServerSession.hpp"

class WorkerThread
{
public:
	WorkerThread(int workerThreadId, HANDLE hThread, HANDLE hCompletionPort)
		: mWorkerThreadId(workerThreadId), mThreadHandle(hThread), mCompletionPort(hCompletionPort)
	{

	}

	~WorkerThread() 
	{
		CloseHandle(mThreadHandle);
	}

	DWORD Run()
	{
		while (true)
		{
			DoIocpJob();

			DoSendJob();
		}

		return 1;
	}

	void DoIocpJob()
	{
		DWORD dwTransferred = 0;
		OverlappedIOContext* context = nullptr;
		ULONG_PTR completionKey = 0;

		int ret = GetQueuedCompletionStatus(mCompletionPort, &dwTransferred, (PULONG_PTR)&completionKey, (LPOVERLAPPED*)&context, 10);

		Session* remote = context ? context->mSessionObject : nullptr;

		if (ret == 0 || dwTransferred == 0)
		{
			if(context == nullptr && GetLastError() == WAIT_TIMEOUT)
			{
				return;
			}

			if (context->mIoType == IOType::IO_RECV || context->mIoType == IOType::IO_SEND)
			{
				CRASH_ASSERT(nullptr != remote);

				remote->DisconnectRequest(DisconnectReason::DR_COMPLETION_ERROR);

				DeleteIoContext(context);

				return;
			}
		}

		CRASH_ASSERT(nullptr != remote);

		bool completionOk = false;
		switch (context->mIoType)
		{
		case IOType::IO_CONNECT:
		{
			dynamic_cast<ServerSession*>(remote)->ConnectCompletion();
		}
		// ...
		}
	}

	void DoSendJob()
	{
		// ...
	}

	HANDLE GetHandle() { return mThreadHandle; }
	
private:
	HANDLE mThreadHandle;
	HANDLE mCompletionPort;
	int mWorkerThreadId;
};
