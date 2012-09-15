#ifndef traktor_online_Score_H
#define traktor_online_Score_H

#include "Core/Object.h"
#include "Core/Ref.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_ONLINE_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace online
	{

class IUser;

class T_DLLCLASS Score : public Object
{
	T_RTTI_CLASS;

public:
	Score(const IUser* user, int32_t score);

	const IUser* getUser() const { return m_user; }

	int32_t getScore() const { return m_score; }

private:
	Ref< const IUser > m_user;
	int32_t m_score;
};

	}
}

#endif	// traktor_online_Score_H
