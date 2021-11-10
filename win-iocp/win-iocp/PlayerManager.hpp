#pragma once
#include "FastSpinLock.hpp"
#include "XTL.hpp"

class Player;
using PlayerList = xvector<std::shared_ptr<Player>>::type;

class PlayerManager
{
public:
	PlayerManager()
		: mLock((int)LockOrder::LO_ECONOMY_CLASS), mCurrentIssueId(0)
	{

	}

	int RegisterPlayer(std::shared_ptr<Player> player)
	{
		FastSpinlockGuard exclusive(mLock);

		mPlayerMap[++mCurrentIssueId] = player;

		return mCurrentIssueId;
	}

	void UnregisterPlayer(int playerId)
	{
		FastSpinlockGuard exclusive(mLock);

		mPlayerMap.erase(playerId);
	}

	int GetCurrentPlayers(PlayerList& outList)
	{
		// TODO: mLock�� read���� �����ؼ� outList�� ���� �÷��̾���� ������ ���
		// total���� ���� �÷��̾��� �� ���� ��ȯ
		mLock.EnterReadLock();
		int total = 0;
		for (const auto& iter : mPlayerMap)
		{
			++total;
			outList.push_back(iter.second);
		}

		return total;
	}

private:
	FastSpinlock mLock;
	int mCurrentIssueId;
	xmap<int, std::shared_ptr<Player>>::type mPlayerMap;
};

extern PlayerManager* GPlayerManager = nullptr;