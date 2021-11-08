#pragma once
#include "predef.hpp"

#include <list>
#include <WinSock2.h>
#include "FastSpinLock.hpp"

#define MAX_CONNECTION 10

SessionManager* GSessionManager = nullptr;

class SessionManager
{
public:
	SessionManager()
		: mCurrentIssueCount{ 0 }, mCurrentReturnCount{ 0 } {}

	~SessionManager()
	{
		for (auto it : mFreeClientList)
		{
			delete it;
		}
	}

	void PrepareSessions()
	{
		CRASH_ASSERT(LThreadType == static_cast<int>(THREAD_TYPE::THREAD_MAIN_ACCEPT));

		for (int i = 0; i < MAX_CONNECTION; ++i)
		{
			ClientSession* client = new ClientSession();

			mFreeClientList.push_back(client);
		}
	}

	void ReturnClientSession(ClientSession* client)
	{
		std::lock_guard<std::mutex> lock(mMutex);

		CRASH_ASSERT(client->mConnected == 0 && client->mRefCount == 0);

		client->SessionReset();

		mFreeClientList.push_back(client);

		++mCurrentReturnCount;
	}

	bool AcceptSessions()
	{
		std::lock_guard<std::mutex> lock(mMutex);

		while (mCurrentIssueCount - mCurrentReturnCount < MAX_CONNECTION)
		{
			//TODO: mFreeSessionList에서 ClientSession* 꺼내서 PostAccept() 해주기 ...
			// AddRef()...

			// 실패시 false return
		}

		return true;
	}

private:
	std::list<ClientSession*> mFreeClientList;

	std::mutex mMutex;

	uint64_t mCurrentIssueCount;
	uint64_t mCurrentReturnCount;
};

extern SessionManager* GSessionManager;