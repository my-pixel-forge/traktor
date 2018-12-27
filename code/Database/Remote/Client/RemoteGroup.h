/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_db_RemoteGroup_H
#define traktor_db_RemoteGroup_H

#include "Database/Provider/IProviderGroup.h"

namespace traktor
{
	namespace db
	{

class RemoteConnection;

/*! \brief Remote group.
 * \ingroup Database
 */
class RemoteGroup : public IProviderGroup
{
	T_RTTI_CLASS;

public:
	RemoteGroup(RemoteConnection* connection, uint32_t handle);

	virtual ~RemoteGroup();

	virtual std::wstring getName() const override final;

	virtual bool rename(const std::wstring& name) override final;

	virtual bool remove() override final;

	virtual Ref< IProviderGroup > createGroup(const std::wstring& groupName) override final;

	virtual Ref< IProviderInstance > createInstance(const std::wstring& instanceName, const Guid& instanceGuid) override final;

	virtual bool getChildren(RefArray< IProviderGroup >& outChildGroups, RefArray< IProviderInstance >& outChildInstances) override final;

private:
	Ref< RemoteConnection > m_connection;
	uint32_t m_handle;
};

	}
}

#endif	// traktor_db_RemoteGroup_H
