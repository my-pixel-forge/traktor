#pragma once

#include <list>
#include <map>
#include <string>
#include "Online/IGameConfiguration.h"

namespace traktor
{
	namespace online
	{

class SteamGameConfiguration : public IGameConfiguration
{
	T_RTTI_CLASS;

public:
	SteamGameConfiguration();

	virtual void serialize(ISerializer& s) override final;

private:
	friend class SteamSessionManager;

	uint32_t m_appId;
	uint32_t m_requestAttempts;
	bool m_drmEnabled;
	bool m_cloudEnabled;
	bool m_allowP2PRelay;
	std::list< std::wstring > m_achievementIds;
	std::list< std::wstring > m_leaderboardIds;
	std::list< std::wstring > m_statsIds;
	std::map< std::wstring, uint32_t > m_dlcIds;
};

	}
}

