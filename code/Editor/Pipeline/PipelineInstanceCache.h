#pragma once

#include "Core/Containers/SmallMap.h"
#include "Core/Thread/Semaphore.h"
#include "Editor/IPipelineInstanceCache.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace db
	{

class Database;

	}

	namespace editor
	{

/*! Pipeline database instance object read-only cache.
 * \ingroup Editor
 */
class T_DLLCLASS PipelineInstanceCache : public IPipelineInstanceCache
{
	T_RTTI_CLASS;

public:
	PipelineInstanceCache(db::Database* database, const std::wstring& cacheDirectory);

	virtual Ref< const ISerializable > getObjectReadOnly(const Guid& instanceGuid) override final;

	virtual void flush(const Guid& instanceGuid) override final;

private:
	struct CacheEntry
	{
		Ref< const ISerializable > object;
		uint32_t hash;
	};

	Semaphore m_lock;
	Ref< db::Database > m_database;
	std::wstring m_cacheDirectory;
	SmallMap< Guid, CacheEntry > m_readCache;
};

	}
}

