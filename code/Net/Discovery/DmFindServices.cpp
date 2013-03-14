#include "Net/Discovery/DmFindServices.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberType.h"

namespace traktor
{
	namespace net
	{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.net.DmFindServices", 1, DmFindServices, IDiscoveryMessage)

DmFindServices::DmFindServices(const Guid& managerGuid)
:	m_managerGuid(managerGuid)
{
}

bool DmFindServices::serialize(ISerializer& s)
{
	if (s.getVersion() < 1)
		return false;

	s >> Member< Guid >(L"managerGuid", m_managerGuid);
	return true;
}

	}
}
