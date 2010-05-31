#include "Core/Io/DynamicMemoryStream.h"
#include "Core/Misc/Base64.h"
#include "Core/Serialization/BinarySerializer.h"
#include "Online/Local/LeaderboardLocal.h"
#include "Online/Local/SaveGameLocal.h"
#include "Online/Local/SessionLocal.h"
#include "Online/Local/UserLocal.h"
#include "Sql/IConnection.h"
#include "Sql/IResultSet.h"

namespace traktor
{
	namespace online
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.online.SessionLocal", SessionLocal, ISession)

SessionLocal::SessionLocal(sql::IConnection* db, UserLocal* user)
:	m_db(db)
,	m_user(user)
{
}

void SessionLocal::destroy()
{
}

bool SessionLocal::isConnected() const
{
	return true;
}

Ref< IUser > SessionLocal::getUser()
{
	return m_user;
}

bool SessionLocal::rewardAchievement(const std::wstring& achievementId)
{
	Ref< sql::IResultSet > rs;

	rs = m_db->executeQuery(L"select id from Achievements where name='" + achievementId + L"'");
	if (!rs || !rs->next())
		return false;

	int32_t aid = rs->getInt32(0);
	int32_t uid = m_user->getId();

	return m_db->executeUpdate(L"insert into AwardedAchievements (achievementId, userId) values (" + toString(aid) + L", " + toString(uid) + L")") >= 0;
}

bool SessionLocal::withdrawAchievement(const std::wstring& achievementId)
{
	Ref< sql::IResultSet > rs;

	rs = m_db->executeQuery(L"select id from Achievements where name='" + achievementId + L"'");
	if (!rs || !rs->next())
		return false;

	int32_t aid = rs->getInt32(0);
	int32_t uid = m_user->getId();

	return m_db->executeUpdate(L"delete from AwardedAchievements where userId=" + toString(uid) + L" and achievementId=" + toString(aid)) >= 0;
}

bool SessionLocal::setStatValue(const std::wstring& statId, float value)
{
	if (m_db->executeUpdate(L"update Stats set value=" + toString(value) + L" where name='" + statId + L"'") > 0)
		return true;

	return m_db->executeUpdate(L"insert into Stats (name, value) values ('" + statId + L"', " + toString(value) + L")") >= 0;
}

Ref< ILeaderboard > SessionLocal::getLeaderboard(const std::wstring& id)
{
	Ref< sql::IResultSet > rs;
	int32_t leaderboardId;

	rs = m_db->executeQuery(L"select id from Leaderboards where name='" + id + L"'");
	if (rs && rs->next())
		leaderboardId = rs->getInt32(0);
	else
	{
		if (m_db->executeUpdate(L"insert into Leaderboards (name) values ('" + id + L"')") < 0)
			return 0;

		leaderboardId = m_db->lastInsertId();
	}

	return new LeaderboardLocal(m_db, leaderboardId, m_user->getId());
}

bool SessionLocal::getStatValue(const std::wstring& statId, float& outValue)
{
	Ref< sql::IResultSet > rs;

	rs = m_db->executeQuery(L"select value from Stats where name='" + statId + L"'");
	if (!rs || !rs->next())
		return false;

	outValue = rs->getFloat(0);
	return true;
}

Ref< ISaveGame > SessionLocal::createSaveGame(const std::wstring& name, ISerializable* attachment)
{
	Ref< sql::IResultSet > rs;
	int32_t id;

	DynamicMemoryStream dms(false, true);
	BinarySerializer(&dms).writeObject(attachment);
	std::wstring ab64 = Base64().encode(dms.getBuffer(), false);

	rs = m_db->executeQuery(L"select id from SaveGames where name='" + name + L"'");
	if (rs && rs->next() && rs->getInt32(0) > 0)
	{
		// Save game already exists; replace attachment.
		id = rs->getInt32(0);
		if (m_db->executeUpdate(L"update SaveGames set attachment='" + ab64 + L"' where id=" + toString(id)) < 0)
			return 0;
	}
	else
	{
		// Doesn't exist; create new save game.
		if (m_db->executeUpdate(L"insert into SaveGames (name, attachment) values ('" + name + L"', '" + ab64 + L"')") < 0)
			return 0;
		id = m_db->lastInsertId();
	}

	return new SaveGameLocal(
		m_db,
		id,
		name
	);
}

bool SessionLocal::getAvailableSaveGames(RefArray< ISaveGame >& outSaveGames) const
{
	Ref< sql::IResultSet > rs;

	rs = m_db->executeQuery(L"select id, name from SaveGames");
	if (!rs)
		return false;

	while (rs->next())
	{
		outSaveGames.push_back(new SaveGameLocal(
			m_db,
			rs->getInt32(0),
			rs->getString(1)
		));
	}

	return true;
}

	}
}
