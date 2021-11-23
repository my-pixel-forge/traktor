#pragma once

#include <list>
#include <map>
#include "Core/Io/Path.h"
#include "Editor/IPipelineBuilder.h"
#include "Editor/PipelineTypes.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

class Thread;

	namespace db
	{

class Group;

	}

	namespace editor
	{

class IPipelineCache;
class IPipelineDb;
class IPipelineInstanceCache;
class PipelineFactory;
class PipelineProfiler;

/*! Pipeline manager.
 * \ingroup Editor
 */
class T_DLLCLASS PipelineBuilder : public IPipelineBuilder
{
	T_RTTI_CLASS;

public:
	explicit PipelineBuilder(
		PipelineFactory* pipelineFactory,
		db::Database* sourceDatabase,
		db::Database* outputDatabase,
		IPipelineCache* cache,
		IPipelineDb* db,
		IPipelineInstanceCache* instanceCache,
		IListener* listener,
		bool verbose
	);

	virtual bool build(const PipelineDependencySet* dependencySet, bool rebuild) override final;

	virtual Ref< ISerializable > buildProduct(const db::Instance* sourceInstance, const ISerializable* sourceAsset, const Object* buildParams) override final;

	virtual bool buildAdHocOutput(const ISerializable* sourceAsset, const Guid& outputGuid, const Object* buildParams) override final;

	virtual bool buildAdHocOutput(const ISerializable* sourceAsset, const std::wstring& outputPath, const Guid& outputGuid, const Object* buildParams) override final;

	virtual uint32_t calculateInclusiveHash(const ISerializable* sourceAsset) const override final;

	virtual Ref< ISerializable > getBuildProduct(const ISerializable* sourceAsset) override final;

	virtual Ref< db::Instance > createOutputInstance(const std::wstring& instancePath, const Guid& instanceGuid) override final;

	virtual db::Database* getOutputDatabase() const override final;

	virtual db::Database* getSourceDatabase() const override final;

	virtual Ref< const ISerializable > getObjectReadOnly(const Guid& instanceGuid) override final;

	virtual DataAccessCache* getDataAccessCache() const override final;

	virtual PipelineProfiler* getProfiler() const override final;

private:
	struct BuiltCacheEntry
	{
		const ISerializable* sourceAsset;
		Ref< ISerializable > product;
	};

	typedef std::list< BuiltCacheEntry > built_cache_list_t;

	Ref< PipelineFactory > m_pipelineFactory;
	Ref< db::Database > m_sourceDatabase;
	Ref< db::Database > m_outputDatabase;
	Ref< IPipelineCache > m_cache;
	Ref< IPipelineCache > m_cacheAdHoc;
	Ref< IPipelineDb > m_pipelineDb;
	Ref< IPipelineInstanceCache > m_instanceCache;
	Ref< DataAccessCache > m_dataAccessCache;
	IListener* m_listener;
	bool m_verbose;
	bool m_rebuild;
	Ref< PipelineProfiler > m_profiler;
	const PipelineDependencySet* m_dependencySet;
	std::map< Guid, Ref< ISerializable > > m_readCache;
	std::map< uint32_t, built_cache_list_t > m_builtCache;
	RefArray< db::Instance >* m_buildInstances;
	int32_t m_adHocDepth;
	int32_t m_progressEnd;
	int32_t m_progress;
	int32_t m_succeeded;
	int32_t m_succeededBuilt;
	int32_t m_failed;
	int32_t m_cacheHit;
	int32_t m_cacheMiss;
	int32_t m_cacheVoid;

	/*! Perform build. */
	BuildResult performBuild(const PipelineDependencySet* dependencySet, const PipelineDependency* dependency, const Object* buildParams, uint32_t reason);

	/*! Isolate instance in cache. */
	bool putInstancesInCache(IPipelineCache* cache, const Guid& guid, const PipelineDependencyHash& hash, const RefArray< db::Instance >& instances);

	/*! Get isolated instance from cache. */
	bool getInstancesFromCache(IPipelineCache* cache, const PipelineDependency* dependency, const PipelineDependencyHash& hash, RefArray< db::Instance >& outInstances);
};

	}
}

