#pragma once
#include "predef.hpp"
#include "Session.hpp"
#include "ObjectPool.hpp"
#include "IocpManager.hpp"

#define SERVER_BUFSIZE (65536*4)

// 다른 서버로의 접속을 위한 세션
class ServerSession : public Session, public ObjectPool<ServerSession>
{
public:
	ServerSession(const char* serverAddr, uint16_t port)
		: Session(SERVER_BUFSIZE, SERVER_BUFSIZE), mServerAddr(serverAddr), mPort(port)
	{

	}

	virtual ~ServerSession() {}

	bool ConnectRequest()
	{
		if (mConnected)
		{
			CRASH_ASSERT(false);
			return false;
		}

		SOCKADDR_IN serverSockAddr;
		ZeroMemory(&serverSockAddr, sizeof(serverSockAddr));
		serverSockAddr.sin_port = htons(mPort);
		serverSockAddr.sin_family = AF_INET;
		serverSockAddr.sin_addr.s_addr = inet_addr(mServerAddr);

		if (SOCKET_ERROR == bind(mSocket, (SOCKADDR*)&serverSockAddr, sizeof(serverSockAddr)))
		{
			// log
			return false;
		}

		HANDLE handle = CreateIoCompletionPort((HANDLE)mSocket, GIocpManager->GetCompletionPort(), (ULONG_PTR)this, 0);
		if (handle != GIocpManager->GetCompletionPort())
		{

		}
	
	
	}

	void ConnectCompletion()
	{

	}

private:
	const char* mServerAddr;
	uint16_t mPort;
};