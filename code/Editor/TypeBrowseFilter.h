#pragma once

#include "Core/Rtti/TypeInfo.h"
#include "Editor/IBrowseFilter.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::editor
{

/*! Filter instances by type.
 * \ingroup Editor
 */
class T_DLLCLASS TypeBrowseFilter : public IBrowseFilter
{
public:
	explicit TypeBrowseFilter(const TypeInfoSet& typeSet);

	explicit TypeBrowseFilter(const TypeInfo& typeInfo);

	virtual bool acceptable(db::Instance* instance) const override final;

private:
	TypeInfoSet m_typeSet;
};

}
