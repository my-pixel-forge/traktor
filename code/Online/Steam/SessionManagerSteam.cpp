#include <steam/steam_api.h>
#include "Core/Log/Log.h"
#include "Core/Misc/TString.h"
#include "Online/Steam/CurrentUserSteam.h"
#include "Online/Steam/SessionManagerSteam.h"
#include "Online/Steam/SessionSteam.h"

namespace traktor
{
	namespace online
	{
		namespace
		{

const struct { const char* steam; const wchar_t* code; } c_languageCodes[] =
{
	{ "english", L"en" },
	{ "french", L"fr" },
	{ "german", L"de" },
	{ "italian", L"it" },
	{ "spanish", L"es" },
	{ "swedish", L"se" }
};

		}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.online.SessionManagerSteam", 0, SessionManagerSteam, ISessionManager)

bool SessionManagerSteam::create()
{
	if (!SteamAPI_Init())
	{
		log::error << L"Unable to initialize Steam API" << Endl;
		return false;
	}

	m_currentUser = new CurrentUserSteam();
	return true;
}

void SessionManagerSteam::destroy()
{
	if (m_session)
	{
		m_session->destroy();
		m_session = 0;
	}

	m_currentUser = 0;

	SteamAPI_Shutdown();
}

std::wstring SessionManagerSteam::getLanguageCode() const
{
	const char* language = SteamApps()->GetCurrentGameLanguage();
	if (!language)
		return L"";

	for (uint32_t i = 0; i < sizeof_array(c_languageCodes); ++i)
	{
		if (_stricmp(language, c_languageCodes[i].steam) == 0)
			return c_languageCodes[i].code;
	}

	log::error << L"Session steam; unable to map language \"" << mbstows(language) << L"\" to ISO 639-1 code" << Endl;
	return L"";
}

bool SessionManagerSteam::getAvailableUsers(RefArray< IUser >& outUsers)
{
	return false;
}

Ref< IUser > SessionManagerSteam::getCurrentUser()
{
	return m_currentUser;
}

Ref< ISession > SessionManagerSteam::createSession(IUser* user)
{
	T_ASSERT (user == m_currentUser);
	T_ASSERT (m_session == 0);

	m_session = new SessionSteam(m_currentUser);
	return m_session;
}

bool SessionManagerSteam::update()
{
	if (m_session)
	{
		if (!m_session->update())
			m_session = 0;
	}

	SteamAPI_RunCallbacks();

	return true;
}

	}
}
