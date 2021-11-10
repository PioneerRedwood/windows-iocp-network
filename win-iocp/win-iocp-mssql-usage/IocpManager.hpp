#pragma once
#include "predef.hpp"
#include <deque>

class Session;
class WorkerThread;

struct OverlappedSendContext;
struct OverlappedPreRecvContext;
struct OverlappedRecvContext;
struct OverlappedDisconnectContext;
struct OverlappedAcceptContext;

IocpManager* GIocpManager = nullptr;

LPFN_DISCONNECTEX IocpManager::mFnDisconnectEx = nullptr;
LPFN_ACCEPTEX IocpManager::mFnAcceptEx = nullptr;
LPFN_CONNECTEX IocpManager::mFnConnectEx = nullptr;

char IocpManager::mAcceptBuf[64] = { 0, };

class IocpManager
{
public:
	IocpManager()
		: mCompletionPort(NULL), mIoThreadCount(2), mListenSocket(NULL)
	{
		memset(mWorkerThread, 0, sizeof(mWorkerThread));
	}

	~IocpManager() {}

	bool Initailze()
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		mIoThreadCount = min(si.dwNumberOfProcessors, MAX_IOTHREAD);

		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		{
			return false;
		}

		mCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (mCompletionPort == NULL)
		{
			return false;
		}

		mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (mListenSocket == INVALID_SOCKET)
		{
			return false;
		}

		HANDLE handle = CreateIoCompletionPort((HANDLE)mListenSocket, mCompletionPort, 0, 0);
		if (handle != mCompletionPort)
		{
			// fail log
			return false;
		}

		int opt = 1;
		setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

		// bind
		SOCKADDR_IN serveraddr;
		ZeroMemory(&serveraddr, 0, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		//serveraddr.sin_port = htons(LISTEN_PORT);
		serveraddr.sin_port = htons(9001);
		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

		if (SOCKET_ERROR == bind(mListenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr)))
		{
			return false;
		}

		GUID guidDisconnectEx = WSAID_DISCONNECTEX;
		DWORD bytes = 0;
		if (SOCKET_ERROR == WSAIoctl(mListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&guidDisconnectEx, sizeof(GUID), &mFnDisconnectEx, sizeof(LPFN_DISCONNECTEX), &bytes, NULL, NULL))
		{
			return false;
		}

		GUID guidAcceptEx = WSAID_ACCEPTEX;
		if (SOCKET_ERROR == WSAIoctl(mListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&guidAcceptEx, sizeof(GUID), &mFnAcceptEx, sizeof(LPFN_ACCEPTEX), &bytes, NULL, NULL))
		{
			return false;
		}

		GUID guidConnectEx = WSAID_CONNECTEX;
		if (SOCKET_ERROR == WSAIoctl(mListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&guidConnectEx, sizeof(GUID), &mFnConnectEx, sizeof(LPFN_CONNECTEX), &bytes, NULL, NULL))
		{
			return false;
		}

		// make session pool
		//GClientSessionManager->PrepareClientSessions();
		return true;
	}

	void Finalize()
	{
		for (int i = 0; i < mIoThreadCount; ++i)
		{
			CloseHandle(mWorkerThread[i]);
		}

		CloseHandle(mCompletionPort);

		// winsock finalizing
		WSACleanup();
	}

	bool StartIoThreads()
	{
		for (int i = 0; i < mIoThreadCount; ++i)
		{
			DWORD dwThreadId;
			HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, IoWorkerThread, (LPVOID)i, CREATE_SUSPENDED, (uint32_t*)&dwThreadId);
			if (hThread == NULL)
			{
				return false;
			}

			mWorkerThread[i] = new WorkerThread(i, hThread, mCompletionPort);
		}

		for (int i = 0; i < mIoThreadCount; ++i)
		{
			ResumeThread(mWorkerThread[i]->GetHandle());
		}

		return true;
	}

	void StartAccept()
	{
		if (SOCKET_ERROR == listen(mListenSocket, SOMAXCONN))
		{
			// listen error log
			return;
		}

		while (GClientSessionManager->AcceptClientSessions())
		{
			Sleep(100);
		}
	}

	HANDLE GetCompletionPort() { return mCompletionPort; }
	int GetIoThreadCount() { return mIoThreadCount; }

	SOCKET* GetListenSocket() { return &mListenSocket; }

	static char mAcceptBuf[64];
	static LPFN_DISCONNECTEX mFnDisconnectEx;
	static LPFN_ACCEPTEX mFnAcceptEx;
	static LPFN_CONNECTEX mFnConnectEx;

private:
	static uint32_t WINAPI IoWorkerThread(LPVOID lpParam)
	{
		LThreadType = (int)THREAD_TYPE::THREAD_IO_WORKER;
		LWorkerThread = reinterpret_cast<int>(lpParam);
		LSendRequestSessionList = new std::deque<Session*>;

		return GIocpManager->mWorkerThread[LWorkerThreadId]->Run();
	}

private:
	HANDLE mCompletionPort;
	int mIoThreadCount;
	SOCKET mListenSocket;

	enum {MAX_IOTHREAD = 8};
	WorkerThread* mWorkerThread[MAX_IOTHREAD];
};

BOOL DisconnectEx(SOCKET hSocket, LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD reserved)
{
	return IocpManager::mFnDisconnectEx(hSocket, lpOverlapped, dwFlags, reserved);
}

BOOL AcceptEx(SOCKET sListenSocket, SOCKET sAcceptSocket, PVOID lpOutputBuffer, DWORD dwReceiveDataLength,
	DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived, LPOVERLAPPED lpOverlapped)
{
	return IocpManager::mFnAcceptEx(sListenSocket, sAcceptSocket, lpOutputBuffer, dwReceiveDataLength,
		dwLocalAddressLength, dwRemoteAddressLength, lpdwBytesReceived, lpOverlapped);
}

BOOL ConnectEx(SOCKET hSocket, const struct sockaddr* name, int namelen, PVOID lpSendBuffer, DWORD dwSendDataLength,
	LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped)
{
	return IocpManager::mFnConnectEx(hSocket, name, namelen, lpSendBuffer, dwSendDataLength, lpdwBytesSent, lpOverlapped);
}

