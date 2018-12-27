/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_RfpMemberName_H
#define traktor_RfpMemberName_H

#include <string>
#include "Core/Reflection/ReflectionMemberPredicate.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_CORE_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

/*! \brief Member name query predicate.
 * \ingroup Core
 */
class T_DLLCLASS RfpMemberName : public ReflectionMemberPredicate
{
	T_RTTI_CLASS;

public:
	RfpMemberName(const std::wstring& memberName);

	virtual bool operator () (const ReflectionMember* member) const override final;

private:
	std::wstring m_memberName;
};

}

#endif	// traktor_RfpMemberName_H
