#ifndef traktor_net_DmServiceInfo_H
#define traktor_net_DmServiceInfo_H

#include "Core/Guid.h"
#include "Net/Discovery/IDiscoveryMessage.h"

namespace traktor
{
	namespace net
	{

class IService;

class DmServiceInfo : public IDiscoveryMessage
{
	T_RTTI_CLASS;

public:
	DmServiceInfo(
		const Guid& managerGuid = Guid(),
		const Guid& serviceGuid = Guid(),
		IService* service = 0
	);

	const Guid& getManagerGuid() const { return m_managerGuid; }

	const Guid& getServiceGuid() const { return m_serviceGuid; }

	Ref< IService > getService() const { return m_service; }

	virtual bool serialize(ISerializer& s);

private:
	Guid m_managerGuid;
	Guid m_serviceGuid;
	Ref< IService > m_service;
};

	}
}

#endif	// traktor_net_DmServiceInfo_H
