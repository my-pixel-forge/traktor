#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/MemberStl.h"
#include "Resource/Editor/ResourceBundleAsset.h"

namespace traktor
{
	namespace resource
	{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"traktor.resource.ResourceBundleAsset", 0, ResourceBundleAsset, ISerializable)

ResourceBundleAsset::ResourceBundleAsset()
:	m_persistent(false)
{
}

const std::vector< Guid >& ResourceBundleAsset::get() const
{
	return m_resources;
}

bool ResourceBundleAsset::persistent() const
{
	return m_persistent;
}

bool ResourceBundleAsset::serialize(ISerializer& s)
{
	s >> MemberStlVector< Guid >(L"resources", m_resources);
	s >> Member< bool >(L"persistent", m_persistent);
	return true;
}

	}
}
