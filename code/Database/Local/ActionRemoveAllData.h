/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_db_ActionRemoveAllData_H
#define traktor_db_ActionRemoveAllData_H

#include "Core/Io/Path.h"
#include "Database/Local/Action.h"

namespace traktor
{

class IStream;

	namespace db
	{

/*! \brief Transaction remove all data action.
 * \ingroup Database
 */
class ActionRemoveAllData : public Action
{
	T_RTTI_CLASS;

public:
	ActionRemoveAllData(const Path& instancePath);

	virtual bool execute(Context* context) override final;

	virtual bool undo(Context* context) override final;

	virtual void clean(Context* context) override final;

	virtual bool redundant(const Action* action) const override final;

private:
	Path m_instancePath;
	std::vector< std::wstring > m_renamedFiles;
};

	}
}

#endif	// traktor_db_ActionRemoveAllData_H
