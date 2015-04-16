#ifndef traktor_net_DmFindServices_H
#define traktor_net_DmFindServices_H

#include "Core/Guid.h"
#include "Net/SocketAddressIPv4.h"
#include "Net/Discovery/IDiscoveryMessage.h"

namespace traktor
{
	namespace net
	{

class DmFindServices : public IDiscoveryMessage
{
	T_RTTI_CLASS;

public:
	DmFindServices();

	DmFindServices(const Guid& managerGuid, const SocketAddressIPv4& replyTo);

	const Guid& getManagerGuid() const { return m_managerGuid; }

	const SocketAddressIPv4& getReplyTo() const { return m_replyTo; }

	virtual void serialize(ISerializer& s);

private:
	Guid m_managerGuid;
	SocketAddressIPv4 m_replyTo;
};

	}
}

#endif	// traktor_net_DmFindServices_H
