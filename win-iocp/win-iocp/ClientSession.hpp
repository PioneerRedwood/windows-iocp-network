#pragma once
#include "predef.hpp"
#include "Exception.hpp"
#include "EduEchoServer.hpp"
#include "ClientSession.hpp"
#include "IocpManager.hpp"
#include "SessionManager.hpp"
#include "CircularBuffer.hpp"

#define BUFSIZE 4096

class ClientSession;
class SessionManager;

enum class IOType : uint32_t
{
	IO_NONE,
	IO_SEND,
	IO_RECV,
	IO_RECV_ZERO,
	IO_ACCEPT,
	IO_DISCONNECT,
};

enum class DisconnectReason : uint32_t
{
	DR_NONE,
	DR_RECV_ZERO,
	DR_ACTIVE,
	DR_ONCONNECT_ERROR,
	DR_COMPLETION_ERROR,
	DR_IO_REQUEST_ERROR,
};

struct OverlappedIOContext
{
	OverlappedIOContext(ClientSession* owner, IOType ioType)
		: mSessionObject(owner), mIoType(ioType)
	{
		std::memset(&mOverlapped, 0, sizeof(OVERLAPPED));
		std::memset(&mWsaBuf, 0, sizeof(WSABUF));
		mSessionObject->AddRef();
	}

	OVERLAPPED mOverlapped;
	// 왜 참조가 아니라 포인터를 이용하는 걸까?
	//ClientSession& mSessionObject;
	ClientSession* mSessionObject;
	IOType mIoType;
	WSABUF mWsaBuf;
	char mBuffer[BUFSIZE];
};

struct OverlappedSendContext : public OverlappedIOContext
{
	OverlappedSendContext(ClientSession* owner) : OverlappedIOContext(owner, IOType::IO_SEND)
	{}
};

struct OverlappedRecvContext : public OverlappedIOContext
{
	OverlappedRecvContext(ClientSession* owner) : OverlappedIOContext(owner, IOType::IO_RECV)
	{}
};

struct OverlappedPreRecvContext : public OverlappedIOContext
{
	OverlappedPreRecvContext(ClientSession* owner) : OverlappedIOContext(owner, IOType::IO_RECV_ZERO)
	{}
};

struct OverlappedDisconnectContext : public OverlappedIOContext
{
	OverlappedDisconnectContext(ClientSession* owner, DisconnectReason dr) 
		: OverlappedIOContext(owner, IOType::IO_DISCONNECT), mDisconnectReason(dr)
	{}

	DisconnectReason mDisconnectReason;
};

struct OverlappedAcceptContext : public OverlappedIOContext
{
	OverlappedAcceptContext(ClientSession* owner) : OverlappedIOContext(owner, IOType::IO_ACCEPT)
	{}
};

void DeleteIoContext(OverlappedIOContext* context)
{

}

class ClientSession
{
public:
	ClientSession()
		: mBuffer{ BUFSIZE }, mConnected{ 0 }, mRefCount{ 0 } {}

	~ClientSession() {}

	void SessionReset() 
	{
		mConnected = 0;
		mRefCount = 0;
		std::memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));

		LINGER lingerOption;
		lingerOption.l_onoff = 1;
		lingerOption.l_linger = 0;
		
		if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (const char*)&lingerOption, sizeof(LINGER)))
		{
			std::cerr << "[DEBUG] setsockopt linger option error: " << GetLastError() << "\n";
		}

		closesocket(mSocket);

		mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	}

	bool OnConnect(SOCKADDR_IN* addr)
	{
		std::lock_guard<std::mutex> lock{ mMutex };

		u_long arg = 1;
		ioctlsocket(mSocket, FIONBIO, &arg);

		int opt = 1;
		setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int));

		opt = 0;
		if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)))
		{
			std::cerr << "[DEBUG] SO_RCVBUF change error: " << GetLastError() << "\n";
			return false;
		}

		// TODO: connect using "CreateIoCompletionPort(HANDLE)mSocket, ...)"
		HANDLE handle = 0;
		if (handle != GIocpManager->GetCompletionPort())
		{
			std::cerr << "[DEBUG] CreateCompletionPort error: " << GetLastError() << "\n";
			return false;
		}

		std::memcpy(&mClientAddr, addr, sizeof(SOCKADDR_IN));
		mConnected = true;

		std::cout << "[DEBUG] Client Connected: IP=" << inet_ntoa(mClientAddr.sin_addr) << 
			", PORT=" << ntohs(mClientAddr.sin_port) << "\n";

		GSessionManager->IncreaseConnectionCount();

		return PostRecv();
	}
	
	bool PostAccept() 
	{
		CRASH_ASSERT(LThreadType == (int)THREAD_TYPE::THREAD_MAIN);

		OverlappedAcceptContext* acceptContext = new OverlappedAcceptContext(this);

		// TODO: AcceptEx
		
		return true;
	}

	void AcceptCompletion() 
	{
		CRASH_ASSERT(LThreadType == (int)THREAD_TYPE::THREAD_IO_WORKER);

		if (1 == InterlockedExchange(&mConnected, 1))
		{
			CRASH_ASSERT(false);
			return;
		}

		bool resultOk = true;
		do
		{
			if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (const char*)GIocpManager->GetListenSocket(), sizeof(SOCKET)))
			{
				std::cerr << "[DEBUG] SO_UPDATE_ACCEPT_CONTEXT error: " << GetLastError();
				resultOk = false;
				break;
			}

			int opt = 1;
			if (SOCKET_ERROR == setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int)))
			{
				std::cerr << "[DEBUG] TCP_NODELAY error: " << GetLastError();
				resultOk = false;
				break;
			}

			opt = 0;
			if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)))
			{
				std::cerr << "[DEBUG] SO_RCVBUF error: " << GetLastError();
				resultOk = false;
				break;
			}

			int addrlen = sizeof(SOCKADDR_IN);
			if (SOCKET_ERROR == getpeername(mSocket, (SOCKADDR*)&mClientAddr, &addrlen))
			{
				std::cerr << "[DEBUG] getpeername error: " << GetLastError();
				resultOk = false;
				break;
			}

			// TODO: CreateCompletionPort connect socket
			//HANDLE handle = CreateIoCompletionPort((HANDLE)mSocket, iocp_handle, completion_key,0)
		} while (false);

		if(!resultOk)
		{
			DisconnectRequest(DisconnectReason::DR_ONCONNECT_ERROR);
			return;
		}

		std::cout << "[DEBUG] Client Connected: IP=" << inet_ntoa(mClientAddr.sin_addr) <<
			", PORT=" << ntohs(mClientAddr.sin_port) << "\n";

		if (false == PreRecv())
		{
			std::cerr << "[DEBUG] PreRecv error: " << GetLastError() << "\n";
		}
	}
	
	void DisconnectRequest(DisconnectReason dr) 
	{
		if (0 == InterlockedExchange(&mConnected, 0))
		{
			return;
		}

		OverlappedDisconnectContext* context = new OverlappedDisconnectContext(this, dr);

		// TODO: DisconnectEx 

	}

	void DisconnectCompletion(DisconnectReason dr) 
	{
		std::cout << "[Debug] Disconnected: Reason=" << static_cast<int>(dr) << 
			" IP=" << inet_ntoa(mClientAddr.sin_addr) << " PORT=" << ntohs(mClientAddr.sin_port) << "\n";

		ReleaseRef();
	}

	bool PreRecv() 
	{
		if (!IsConnected())
		{
			return false;
		}

		OverlappedRecvContext* context = new OverlappedRecvContext(this);

		// TODO: zero-byte recv 

		return true;
	}

	bool IsConnected() const { return mConnected; }

	bool PostRecv()
	{
		if (!IsConnected())
		{
			return false;
		}

		// wrapped by lock_guard
		{
			std::lock_guard<std::mutex> lock(mMutex);
			OverlappedRecvContext* context = new OverlappedRecvContext(this);

			/*
			int
			WSAAPI
			WSARecv(
				_In_ SOCKET s,
				_In_reads_(dwBufferCount) __out_data_source(NETWORK) LPWSABUF lpBuffers,
				_In_ DWORD dwBufferCount,
				_Out_opt_ LPDWORD lpNumberOfBytesRecvd,
				_Inout_ LPDWORD lpFlags,
				_Inout_opt_ LPWSAOVERLAPPED lpOverlapped,
				_In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
				);
			*/

			DWORD recvBytes = 0;
			DWORD flags = 0;
			context->mWsaBuf.len = (ULONG)mBuffer.GetFreeSpaceSize();
			context->mWsaBuf.buf = mBuffer.GetBuffer();

			if (SOCKET_ERROR == WSARecv(mSocket, &context->mWsaBuf, 1, &recvBytes, &flags, (LPWSAOVERLAPPED)context, NULL))
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					DeleteIoContext(context);
					std::cerr << "ClientSession::PostRecv Error: " << GetLastError() << "\n";
					return false;
				}
			}
		}

		return true;
	}

	void RecvCompletion(DWORD transferred) 
	{
		std::lock_guard<std::mutex> lock(mMutex);

		mBuffer.Commit(transferred);
	}
	
	bool PostSend()
	{
		if (!IsConnected())
		{
			return false;
		}

		{
			std::lock_guard<std::mutex> lock(mMutex);

			OverlappedSendContext* context = new OverlappedSendContext(this);

			DWORD sendBytes = 0;
			DWORD flags = 0;
			context->mWsaBuf.len = (ULONG)mBuffer.GetContiguiousBytes();
			context->mWsaBuf.buf = mBuffer.GetBufferStart();

			// async read
			if (SOCKET_ERROR == WSASend(mSocket, &context->mWsaBuf, 1, &sendBytes, flags, (LPWSAOVERLAPPED)context, NULL))
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					DeleteIoContext(context);
					std::cerr << "ClientSession::PostSend Error: " << GetLastError() << "\n";
					return false;
				}
			}
		}

		return true;
	}
	
	void SendCompletion(DWORD transferred) 
	{
		std::lock_guard<std::mutex> lock(mMutex);

		mBuffer.Remove(transferred);
	}

	void AddRef()
	{
		CRASH_ASSERT(InterlockedIncrement(&mRefCount) > 0);
	}

	void ReleaseRef()
	{
		long ref = InterlockedDecrement(&mRefCount);
		CRASH_ASSERT(ref >= 0);

		if (ref == 0)
		{
			GSessionManager->CreateClientSession(this);
		}
	}

	void SetSocket(SOCKET sock) { mSocket = sock; }
	SOCKET GetSocket() const { return mSocket; }

private:
	SOCKET mSocket;
	SOCKADDR_IN mClientAddr;

	// FastSpinlock vs mutex
	//FastSpinlock mBufferLock;
	std::mutex mMutex;

	CircularBuffer mBuffer;

	volatile long mRefCount;
	volatile long mConnected;

	friend class SessionManager;
};

