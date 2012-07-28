#ifndef traktor_online_SteamMatchMaking_H
#define traktor_online_SteamMatchMaking_H

#include <steam/steam_api.h>
#include "Online/Provider/IMatchMakingProvider.h"

namespace traktor
{
	namespace online
	{

class SteamSessionManager;

class SteamMatchMaking : public IMatchMakingProvider
{
	T_RTTI_CLASS;

public:
	SteamMatchMaking(SteamSessionManager* sessionManager);

	virtual bool findMatchingLobbies(const LobbyFilter* filter, std::vector< uint64_t >& outLobbyHandles);

	virtual bool createLobby(uint32_t maxUsers, uint64_t& outLobbyHandle);

	virtual bool joinLobby(uint64_t lobbyHandle);

	virtual bool leaveLobby(uint64_t lobbyHandle);

	virtual bool setMetaValue(uint64_t lobbyHandle, const std::wstring& key, const std::wstring& value);

	virtual bool getMetaValue(uint64_t lobbyHandle, const std::wstring& key, std::wstring& outValue);

	virtual bool setParticipantMetaValue(uint64_t lobbyHandle, const std::wstring& key, const std::wstring& value);

	virtual bool getParticipantMetaValue(uint64_t lobbyHandle, uint64_t userHandle, const std::wstring& key, std::wstring& outValue);

	virtual bool getParticipants(uint64_t lobbyHandle, std::vector< uint64_t >& outUserHandles);

	virtual bool getParticipantCount(uint64_t lobbyHandle, uint32_t& outCount) const;

	virtual bool getIndex(uint64_t lobbyHandle, int32_t& outIndex) const;

private:
	SteamSessionManager* m_sessionManager;
	std::vector< uint64_t >* m_outLobbies;
	uint64_t* m_outLobby;
	bool m_joinResult;
	CCallResult< SteamMatchMaking, LobbyMatchList_t > m_callbackLobbyMatch;
	CCallResult< SteamMatchMaking, LobbyCreated_t > m_callbackLobbyCreated;
	CCallResult< SteamMatchMaking, LobbyEnter_t > m_callbackLobbyEnter;

	void OnLobbyMatch(LobbyMatchList_t* pCallback, bool bIOFailure);

	void OnLobbyCreated(LobbyCreated_t* pCallback, bool bIOFailure);

	void OnLobbyEnter(LobbyEnter_t* pCallback, bool bIOFailure);
};

	}
}

#endif	// traktor_online_SteamMatchMaking_H
