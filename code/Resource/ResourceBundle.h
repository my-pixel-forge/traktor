#pragma once

#include "Core/Guid.h"
#include "Core/Serialization/ISerializable.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_RESOURCE_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace resource
	{

/*! \brief
 * \ingroup Resource
 */
class T_DLLCLASS ResourceBundle : public ISerializable
{
	T_RTTI_CLASS;

public:
	ResourceBundle();

	ResourceBundle(const std::vector< std::pair< const TypeInfo*, Guid > >& resources, bool persistent);

	const std::vector< std::pair< const TypeInfo*, Guid > >& get() const;

	bool persistent() const;

	virtual void serialize(ISerializer& s) override final;

private:
	std::vector< std::pair< const TypeInfo*, Guid > > m_resources;
	bool m_persistent;
};

	}
}

