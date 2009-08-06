#ifndef traktor_db_ConnectionManager_H
#define traktor_db_ConnectionManager_H

#include "Core/Heap/Ref.h"
#include "Core/Object.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_DATABASE_REMOTE_SERVER_EXPORT)
#define T_DLLCLASS T_DLLEXPORT
#else
#define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace net
	{

class TcpSocket;

	}

	namespace db
	{

class Configuration;
class Connection;

/*! \brief
 */
class T_DLLCLASS ConnectionManager : public Object
{
	T_RTTI_CLASS(ConnectionManager)

public:
	bool create(const Configuration* configuration);

	void destroy();

	bool update(int32_t waitTimeout = 1000);

private:
	Ref< const Configuration > m_configuration;
	Ref< net::TcpSocket > m_serverSocket;
	RefArray< Connection > m_connections;

	bool acceptConnections(int32_t waitTimeout);

	bool cleanupConnections();
};

	}
}

#endif	// traktor_db_ConnectionManager_H
