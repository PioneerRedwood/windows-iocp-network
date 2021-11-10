#pragma once
#include "predef.hpp"
#include "SyncExecutable.hpp"
#include "Timer.hpp"
#include "ThreadLocal.hpp"
#include "ClientSession.hpp"
#include "PlayerManager.hpp"
#include "PlayerWideEvent.hpp"
#include "GrandCentralExecutor.hpp"

class ClientSession;

class Player : public SyncExecutable
{
public:
	Player(ClientSession* session)
		: mSession(session)
	{
		PlayerReset();
	}
	virtual ~Player() 
	{
		CRASH_ASSERT(false);
	}

	int GetPlayerId() { return mPlayerId; }
	bool IsAlive() { return mIsAlive; }
	void Start(int heartbeat)
	{
		mIsAlive = true;
		mHeartBeat = heartbeat;

		mPlayerId = GPlayerManager->RegisterPlayer(GetSharedFromThis<Player>());

		OnTick();
	}

	void OnTick()
	{
		if (!IsAlive())
		{
			return;
		}

		if (rand() % 100 == 0)
		{
			int buffId = mPlayerId * 100;
			int duration = (rand() % 3 + 2) * 1000;

			auto playerEvent = std::make_shared<AllPlayerBuffEvent>(buffId, duration);
			GCEDispatch(playerEvent, &AllPlayerBuffEvent::DoBuffAllPlayers, mPlayerId);
		}

		// TODO: AllPlayerBuffDecay::CheckBuffTimeout를 GrandCetralExecutor를 통해 실행

		if (mHeartBeat > 0)
		{
			DoSyncAfter(mHeartBeat, GetSharedFromThis<Player>(), &Player::OnTick);
		}
	}

	void PlayerReset()
	{
		GPlayerManager->UnregisterPlayer(mPlayerId);

		mPlayerId = -1;
		mHeartBeat = -1;
		mIsAlive = false;
	}

	void AddBuff(int fromPlayerId, int buffId, int duration)
	{
		mBuffList.insert(std::make_pair(buffId, duration));
	}

	void DecayTickBuff()
	{
		for (auto it = mBuffList.begin(); it != mBuffList.end();)
		{
			it->second -= mHeartBeat;

			if (it->second <= 0)
			{
				mBuffList.erase(it++);
			}
			else
			{
				++it;
			}
		}
	}

private:
	int mPlayerId;
	int mHeartBeat;
	bool mIsAlive;

	std::map<int, int> mBuffList;
	ClientSession* const mSession;
	friend class ClientSession;
};