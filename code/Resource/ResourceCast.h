#pragma once

#include "Core/Class/Any.h"
#include "Resource/Proxy.h"
#include "Resource/ResourceHandle.h"

namespace traktor
{

/*! Cast resource proxy.
 * \ingroup Resource
 */
template < typename ResourceType >
struct CastAny < resource::Proxy< ResourceType >, false >
{
	static std::wstring typeName()
	{
		return str(L"resource::Proxy< %ls >", type_name< ResourceType >());
	}

	static bool accept(const Any& value)
	{
		return value.isObject< resource::ResourceHandle >();
	}

	static Any set(const resource::Proxy< ResourceType >& value)
	{
		return Any::fromObject(value.getHandle());
	}

	static resource::Proxy< ResourceType > get(const Any& value)
	{
		resource::ResourceHandle* handle = dynamic_type_cast< resource::ResourceHandle* >(value.getObject());
		if (handle)
			return resource::Proxy< ResourceType >(handle);
		else
			return resource::Proxy< ResourceType >();
	}
};

/*! Cast resource proxy.
 * \ingroup Resource
 */
template < typename ResourceType >
struct CastAny < const resource::Proxy< ResourceType >&, false >
{
	static std::wstring typeName()
	{
		return str(L"resource::Proxy< %ls >", type_name< ResourceType >());
	}

	static bool accept(const Any& value)
	{
		return value.isObject< resource::ResourceHandle >();
	}

	static Any set(const resource::Proxy< ResourceType >& value)
	{
		return Any::fromObject(value.getHandle());
	}

	static resource::Proxy< ResourceType > get(const Any& value)
	{
		resource::ResourceHandle* handle = dynamic_type_cast< resource::ResourceHandle* >(value.getObject());
		if (handle)
			return resource::Proxy< ResourceType >(handle);
		else
			return resource::Proxy< ResourceType >();
	}
};

}
