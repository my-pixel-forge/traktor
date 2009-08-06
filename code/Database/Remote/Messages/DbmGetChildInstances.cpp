#include "Database/Remote/Messages/DbmGetChildInstances.h"
#include "Core/Serialization/Serializer.h"
#include "Core/Serialization/Member.h"

namespace traktor
{
	namespace db
	{

T_IMPLEMENT_RTTI_SERIALIZABLE_CLASS(L"traktor.db.DbmGetChildInstances", DbmGetChildInstances, IMessage)

DbmGetChildInstances::DbmGetChildInstances(uint32_t handle)
:	m_handle(handle)
{
}

bool DbmGetChildInstances::serialize(Serializer& s)
{
	return s >> Member< uint32_t >(L"handle", m_handle);
}

	}
}
