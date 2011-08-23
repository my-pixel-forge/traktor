#ifndef traktor_db_RemoteInstance_H
#define traktor_db_RemoteInstance_H

#include "Database/Provider/IProviderInstance.h"

namespace traktor
{
	namespace db
	{

class RemoteConnection;

/*! \brief Remote instance.
 * \ingroup Database
 */
class RemoteInstance : public IProviderInstance
{
	T_RTTI_CLASS;

public:
	RemoteInstance(RemoteConnection* connection, uint32_t handle);

	virtual ~RemoteInstance();

	virtual std::wstring getPrimaryTypeName() const;

	virtual bool openTransaction();

	virtual bool commitTransaction();

	virtual bool closeTransaction();

	virtual std::wstring getName() const;

	virtual bool setName(const std::wstring& name);

	virtual Guid getGuid() const;

	virtual bool setGuid(const Guid& guid);

	virtual bool remove();

	virtual Ref< IStream > readObject(const TypeInfo*& outSerializerType);

	virtual Ref< IStream > writeObject(const std::wstring& primaryTypeName, const TypeInfo*& outSerializerType);

	virtual uint32_t getDataNames(std::vector< std::wstring >& outDataNames) const;

	virtual bool removeAllData();

	virtual Ref< IStream > readData(const std::wstring& dataName);

	virtual Ref< IStream > writeData(const std::wstring& dataName);

private:
	Ref< RemoteConnection > m_connection;
	uint32_t m_handle;
};

	}
}

#endif	// traktor_db_RemoteInstance_H
