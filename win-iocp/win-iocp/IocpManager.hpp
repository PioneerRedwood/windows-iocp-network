#pragma once
#include "predef.hpp"
#include "EduEchoServer.hpp"
#include "ClientSession.hpp"
#include "SessionManager.hpp"

#define GQCS_TIMEOUT 20

//__declspec(thread) int LIoThreadId = 0;
//IocpManager* GIocpManager = nullptr;

class ClientSession;
struct OverlappedSendContext;
struct OverlappedPreRecvContext;
struct OverlappedRecvContext;
struct OverlappedDisconnectContext;
struct OverlappedAcceptContext;

class IocpManager
{
public:
	IocpManager()
		: mCompletionPort{ nullptr }, mIoThreadCount{ 2 }, mListenSocket{ NULL } {}
	~IocpManager() { };

	bool Initialize()
	{
		// TODO: using GetSystemInfo set num of IO threads
		LPSYSTEM_INFO info;
		GetSystemInfo(info);
		mIoThreadCount = info->dwNumberOfProcessors;

		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		{
			return false;
		}

		// Create IO CompletionPort
		// TODO: mCompletionPort = CreateIoCompletionPort(...)
		/*
		HANDLE
		WINAPI
		CreateIoCompletionPort(
			_In_ HANDLE FileHandle,
			_In_opt_ HANDLE ExistingCompletionPort,
			_In_ ULONG_PTR CompletionKey,
			_In_ DWORD NumberOfConcurrentThreads
			);
		*/
		mCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

		// Create TCP socket
		// TODO: mListenSocket = ...
		mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (mListenSocket == INVALID_SOCKET)
		{
			return false;
		}

		HANDLE handle = CreateIoCompletionPort((HANDLE)mListenSocket, mCompletionPort, 0, 0);
		if (handle != mCompletionPort)
		{
			std::cerr << "[DEBUG] listen socket IOCP register error: " << GetLastError();
			return false;
		}

		int opt = 1;
		setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

		// TODO: bind
		SOCKADDR_IN serveraddr;
		ZeroMemory(&serveraddr, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons(LISTEN_PORT);
		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
				
		if (SOCKET_ERROR == bind(mListenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr)))
		{
			 return false;
		}

		// TODO: WSAIoctl AcceptEx DisconnectEx

		GSessionManager->PrepareSessions();
		return true;
	}

	void Finalize()
	{
		CloseHandle(mCompletionPort);

		// winsock finalizing
		WSACleanup();
	}

	bool StartIoThreads()
	{
		// IO Thread
		for (int i = 0; i < mIoThreadCount; ++i)
		{
			DWORD dwThreadId;
			// TODO: Handle hThread = (HANDLE)_beginthreadex...);
			/*
			_ACRTIMP uintptr_t __cdecl _beginthreadex(
				_In_opt_  void*                    _Security,
				_In_      unsigned                 _StackSize,
				_In_      _beginthreadex_proc_type _StartAddress,
				_In_opt_  void*                    _ArgList,
				_In_      unsigned                 _InitFlag,
				_Out_opt_ unsigned*                _ThrdAddr
				);
			*/
			HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, IoWorkerThread, (LPVOID)(i + 1), 0, (uint32_t*)&dwThreadId);
			if (hThread == NULL)
			{
				return false;
			}
		}

		return true;
	}

	bool StartAccept()
	{
		// listen
		if (SOCKET_ERROR == listen(mListenSocket, SOMAXCONN))
		{
			return false;
		}

		// accept loop
		while (GSessionManager->AcceptSessions())
		{
			Sleep(100);
		}

		return true;
	}

	HANDLE GetCompletionPort() {return mCompletionPort; }
	int GetIoThreadCount() {return mIoThreadCount; }

	SOCKET* GetListenSocket() { return &mListenSocket; }
	
private:
	// IO 작업 스레드
	static unsigned int WINAPI IoWorkerThread(LPVOID lpParam)
	{
		LThreadType = static_cast<int>(THREAD_TYPE::THREAD_IO_WORKER);

		LIoThreadId = reinterpret_cast<int>(lpParam);
		HANDLE hCompletionPort = GIocpManager->GetCompletionPort();

		while (true)
		{
			DWORD dwTransferred = 0;
			OverlappedIOContext* context = nullptr;
			ULONG_PTR completionKey = 0;


			//GetQueuedCompletionStatus(hCompletionPort, ... ,GQCS_TIMEOUT) conducted result
			int ret = GetQueuedCompletionStatus(hCompletionPort, &dwTransferred, (PULONG_PTR)&completionKey, (LPOVERLAPPED*)&context, GQCS_TIMEOUT);

			ClientSession* theClient = context ? context->mSessionObject : nullptr;
			
			if (ret == 0 || dwTransferred == 0)
			{
				int gle = GetLastError();
				// TODO: check time out first ... GQCS time out??..

				if (context->mIoType == IOType::IO_RECV || context->mIoType == IOType::IO_SEND)
				{
					CRASH_ASSERT(nullptr != theClient);

					theClient->DisconnectRequest(DisconnectReason::DR_COMPLETION_ERROR);

					DeleteIoContext(context);

					continue;
				}
			}

			CRASH_ASSERT(nullptr != theClient);

			bool completionOk = false;
			switch (context->mIoType)
			{
			case IOType::IO_DISCONNECT:
			{
				theClient->DisconnectCompletion(static_cast<OverlappedDisconnectContext*>(context)->mDisconnectReason);
				completionOk = true;
				break;
			}
			case IOType::IO_ACCEPT:
			{
				theClient->AcceptCompletion();
				completionOk = true;
				break;
			}
			case IOType::IO_RECV_ZERO:
			{
				completionOk = PreReceiveCompletion(theClient, static_cast<OverlappedPreRecvContext*>(context), dwTransferred);
				break;
			}
			case IOType::IO_SEND:
			{
				completionOk = SendCompletion(theClient, static_cast<OverlappedSendContext*>(context), dwTransferred);
				break;
			}
			case IOType::IO_RECV:
			{
				completionOk = ReceiveCompletion(theClient, static_cast<OverlappedRecvContext*>(context), dwTransferred);
				break;
			}
			default:
			{
				std::cout << "Unknown I/O Type: " << (uint32_t)context->mIoType << "\n";
				CRASH_ASSERT(false);
				break;
			}
			}

			if (!completionOk)
			{
				// connection closing
				theClient->DisconnectRequest(DisconnectReason::DR_IO_REQUEST_ERROR);
			}

			DeleteIoContext(context);
		}
		return 0;
	}
	
	static bool PreReceiveCompletion(ClientSession* client, OverlappedPreRecvContext* context, DWORD dwTransferred)
	{
		return client->PreRecv();
	}

	static bool ReceiveCompletion(ClientSession* client, OverlappedRecvContext* context, DWORD dwTransferred)
	{
		client->RecvCompletion(dwTransferred);

		return client->PostSend();
	}

	static bool SendCompletion(ClientSession* client, OverlappedSendContext* context, DWORD dwTransferred)
	{
		client->SendCompletion(dwTransferred);
		
		if (context->mWsaBuf.len != dwTransferred)
		{
			std::cout << "Partial SendCompletion requested [" << context->mWsaBuf.len << "], sent [" << dwTransferred << "]\n";
			return false;
		}

		return client->PreRecv();
	}

private:
	HANDLE mCompletionPort;
	int mIoThreadCount;
	SOCKET mListenSocket;
};

extern __declspec(thread) int LIoThreadId;
extern IocpManager* GIocpManager;