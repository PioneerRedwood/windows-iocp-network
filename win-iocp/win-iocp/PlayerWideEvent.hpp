#pragma once

class AllPlayerBuffEvent
{
public:
	AllPlayerBuffEvent(int buffId, int duration)
		: mBuffId(buffId), mDuration(duration)
	{

	}

	void DoBuffAllPlayers(int fromPlayerId)
	{
		PlayerList currentPlayers;

		if (GPlayerManager->GetCurrentPlayers(currentPlayers) > 0)
		{
			for (const auto& it : currentPlayers)
			{
				it->AddBuff(fromPlayerId, mBuffId, mDuration);
			}
		}
	}

private:
	int mBuffId;
	int mDuration;
};

class AllPlayerBuffDecay
{
public:
	AllPlayerBuffDecay() {}

	void CheckBuffTimeout()
	{
		PlayerList currentPlayers;

		if (GPlayerManager->GetCurrentPlayers(currentPlayers) > 0)
		{
			for (auto& it : currentPlayers)
			{
				it->DecayTickBuff();
			}
		}
	}
};