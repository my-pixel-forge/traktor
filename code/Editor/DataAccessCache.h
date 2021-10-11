#pragma once

#include <functional>
#include "Core/Object.h"
#include "Core/Ref.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

class IStream;

	namespace editor
	{

class IPipelineCache;

/*! Data access cache.
 *
 * This cache is useful for storing expensive operations, aka memoization,
 * from previous runs.
 */
class T_DLLCLASS DataAccessCache : public Object
{
	T_RTTI_CLASS;

public:
	explicit DataAccessCache(IPipelineCache* cache);

	template< typename ObjectType >
	Ref< ObjectType > read(
		uint32_t key,
		const std::function< Ref< ObjectType > (IStream* stream) >& read,
		const std::function< bool (const ObjectType* object, IStream* stream) >& write,
		const std::function< Ref< ObjectType > () >& create
	)
	{
		return dynamic_type_cast< ObjectType* >(readObject(
			key,
			[&](IStream* stream) -> Ref< Object > { return read(stream); },
			[=](const Object* object, IStream* stream) -> bool { return write(mandatory_non_null_type_cast< const ObjectType* >(object), stream); },
			[&]() -> Ref< Object > { return dynamic_type_cast< ObjectType* >(create()); }
		));
	}

private:
	IPipelineCache* m_cache;

	Ref< Object > readObject(
		uint32_t key,
		const std::function< Ref< Object > (IStream* stream) >& read,
		const std::function< bool (const Object* object, IStream* stream) >& write,
		const std::function< Ref< Object > () >& create
	);
};

	}
}