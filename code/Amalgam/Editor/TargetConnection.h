#ifndef traktor_amalgam_TargetConnection_H
#define traktor_amalgam_TargetConnection_H

#include "Amalgam/Impl/TargetPerformance.h"
#include "Core/Object.h"

namespace traktor
{
	namespace net
	{

class TcpSocket;

	}

	namespace amalgam
	{

class TargetConnection : public Object
{
	T_RTTI_CLASS;

public:
	TargetConnection(net::TcpSocket* socket);

	void destroy();

	bool update();

	net::TcpSocket* getSocket() const { return m_socket; }

	const TargetPerformance& getPerformance() const { return m_performance; }

	const TargetPerformance& getDeltaPerformance() const { return m_deltaPerformance; }

private:
	Ref< net::TcpSocket > m_socket;
	TargetPerformance m_performance;
	TargetPerformance m_deltaPerformance;
};

	}
}

#endif	// traktor_amalgam_TargetConnection_H
