#include "Core/Io/BufferedStream.h"
#include "Core/Io/FileSystem.h"
#include "Core/Log/Log.h"
#include "Core/Misc/Adler32.h"
#include "Core/Misc/Save.h"
#include "Core/Serialization/ISerializable.h"
#include "Core/Thread/Thread.h"
#include "Core/Thread/ThreadManager.h"
#include "Database/Database.h"
#include "Database/Instance.h"
#include "Editor/IPipeline.h"
#include "Editor/IPipelineDb.h"
#include "Editor/PipelineDependencySet.h"
#include "Editor/IPipelineInstanceCache.h"
#include "Editor/PipelineDependency.h"
#include "Editor/Pipeline/PipelineDependsIncremental.h"
#include "Editor/Pipeline/PipelineFactory.h"

namespace traktor
{
	namespace editor
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.editor.PipelineDependsIncremental", PipelineDependsIncremental, IPipelineDepends)

PipelineDependsIncremental::PipelineDependsIncremental(
	PipelineFactory* pipelineFactory,
	db::Database* sourceDatabase,
	db::Database* outputDatabase,
	PipelineDependencySet* dependencySet,
	IPipelineDb* pipelineDb,
	IPipelineInstanceCache* instanceCache,
	uint32_t recursionDepth
)
:	m_pipelineFactory(pipelineFactory)
,	m_sourceDatabase(sourceDatabase)
,	m_outputDatabase(outputDatabase)
,	m_dependencySet(dependencySet)
,	m_pipelineDb(pipelineDb)
,	m_instanceCache(instanceCache)
,	m_maxRecursionDepth(recursionDepth)
,	m_currentRecursionDepth(0)
,	m_result(true)
{
}

void PipelineDependsIncremental::addDependency(const ISerializable* sourceAsset)
{
	if (!sourceAsset || !m_result)
		return;

	// Don't add dependency if thread is about to be stopped.
	if (ThreadManager::getInstance().getCurrentThread()->stopped())
		return;

	const TypeInfo* pipelineType;
	uint32_t pipelineHash;

	if (!m_pipelineFactory->findPipelineType(type_of(sourceAsset), pipelineType, pipelineHash))
	{
		log::error << L"Unable to add dependency to source asset (" << type_name(sourceAsset) << L"); no pipeline found." << Endl;
		m_result = false;
		return;
	}

	Ref< IPipeline > pipeline = m_pipelineFactory->findPipeline(*pipelineType);
	T_ASSERT(pipeline);

	pipeline->buildDependencies(this, nullptr, sourceAsset, L"", Guid());

	if (m_currentDependency)
		m_currentDependency->pipelineHash += pipelineHash;
}

void PipelineDependsIncremental::addDependency(const ISerializable* sourceAsset, const std::wstring& outputPath, const Guid& outputGuid, uint32_t flags)
{
	if (!sourceAsset || !m_result)
		return;

	// Don't add dependency if thread is about to be stopped.
	if (ThreadManager::getInstance().getCurrentThread()->stopped())
		return;

	// Don't add dependency multiple times.
	uint32_t dependencyIndex = m_dependencySet->get(outputGuid);
	if (dependencyIndex != PipelineDependencySet::DiInvalid)
	{
		PipelineDependency* dependency = m_dependencySet->get(dependencyIndex);
		T_ASSERT(dependency);

		dependency->flags |= flags;
		if (m_currentDependency)
			m_currentDependency->children.insert(dependencyIndex);

		return;
	}

	addUniqueDependency(
		nullptr,
		sourceAsset,
		outputPath,
		outputGuid,
		flags
	);
}

void PipelineDependsIncremental::addDependency(db::Instance* sourceAssetInstance, uint32_t flags)
{
	if (!sourceAssetInstance || !m_result)
		return;

	// Don't add dependency if thread is about to be stopped.
	if (ThreadManager::getInstance().getCurrentThread()->stopped())
		return;

	// Don't add dependency multiple times.
	uint32_t dependencyIndex = m_dependencySet->get(sourceAssetInstance->getGuid());
	if (dependencyIndex != PipelineDependencySet::DiInvalid)
	{
		PipelineDependency* dependency = m_dependencySet->get(dependencyIndex);
		T_ASSERT(dependency);

		dependency->flags |= flags;
		if (m_currentDependency)
			m_currentDependency->children.insert(dependencyIndex);

		return;
	}

	// Checkout source asset instance.
	Ref< const ISerializable > sourceAsset = m_instanceCache->getObjectReadOnly(sourceAssetInstance->getGuid());
	if (!sourceAsset)
	{
		log::error << L"Unable to add dependency to \"" << sourceAssetInstance->getName() << L"\"; failed to checkout instance." << Endl;
		m_result = false;
		return;
	}

	addUniqueDependency(
		sourceAssetInstance,
		sourceAsset,
		sourceAssetInstance->getPath(),
		sourceAssetInstance->getGuid(),
		flags
	);
}

void PipelineDependsIncremental::addDependency(const Guid& sourceAssetGuid, uint32_t flags)
{
	if (sourceAssetGuid.isNull() || !sourceAssetGuid.isValid() || !m_result)
		return;

	// Don't add dependency if thread is about to be stopped.
	if (ThreadManager::getInstance().getCurrentThread()->stopped())
		return;

	// Don't add dependency multiple times.
	uint32_t dependencyIndex = m_dependencySet->get(sourceAssetGuid);
	if (dependencyIndex != PipelineDependencySet::DiInvalid)
	{
		PipelineDependency* dependency = m_dependencySet->get(dependencyIndex);
		T_ASSERT(dependency);

		dependency->flags |= flags;
		if (m_currentDependency)
			m_currentDependency->children.insert(dependencyIndex);

		return;
	}

	// Get source asset instance from database.
	Ref< db::Instance > sourceAssetInstance = m_sourceDatabase->getInstance(sourceAssetGuid);
	if (!sourceAssetInstance)
	{
		// \hack
		// In case output database already contain an instance with given ID we assume it has
		// been synthesized.
		if (m_outputDatabase->getInstance(sourceAssetGuid) == nullptr)
		{
			if (m_currentDependency)
				log::error << L"Unable to add dependency to \"" << sourceAssetGuid.format() << L"\"; no such instance (referenced by \"" << m_currentDependency->sourceInstanceGuid.format() << L"\")." << Endl;
			else
				log::error << L"Unable to add dependency to \"" << sourceAssetGuid.format() << L"\"; no such instance." << Endl;
			m_result = false;
		}
		return;
	}

	// Checkout source asset instance.
	Ref< const ISerializable > sourceAsset = m_instanceCache->getObjectReadOnly(sourceAssetGuid);
	if (!sourceAsset)
	{
		if (m_currentDependency)
			log::error << L"Unable to add dependency to \"" << sourceAssetGuid.format() << L"\"; failed to checkout instance (referenced by \"" << m_currentDependency->sourceInstanceGuid.format() << L"\")." << Endl;
		else
			log::error << L"Unable to add dependency to \"" << sourceAssetGuid.format() << L"\"; failed to checkout instance." << Endl;
		m_result = false;
		return;
	}

	addUniqueDependency(
		sourceAssetInstance,
		sourceAsset,
		sourceAssetInstance->getPath(),
		sourceAssetInstance->getGuid(),
		flags
	);
}

void PipelineDependsIncremental::addDependency(
	const Path& basePath,
	const std::wstring& fileName
)
{
	if (!m_result)
		return;

	Path filePath = FileSystem::getInstance().getAbsolutePath(basePath, fileName);
	if (m_currentDependency)
	{
		Ref< File > file = FileSystem::getInstance().get(filePath);
		if (file)
		{
			PipelineDependency::ExternalFile externalFile;
			externalFile.filePath = filePath;
			externalFile.lastWriteTime = file->getLastWriteTime();
			m_currentDependency->files.push_back(externalFile);
		}
		else
		{
			log::error << L"Unable to add dependency to \"" << filePath.getPathName() << L"\"; no such file." << Endl;
			m_result = false;
		}
	}
}

void PipelineDependsIncremental::addDependency(
	const TypeInfo& sourceAssetType
)
{
	if (!m_result)
		return;

	// Find pipeline which consume asset type.
	const TypeInfo* pipelineType;
	uint32_t pipelineHash;

	if (!m_pipelineFactory->findPipelineType(sourceAssetType, pipelineType, pipelineHash))
	{
		log::error << L"Unable to add dependency to source asset (" << sourceAssetType.getName() << L"); no pipeline found." << Endl;
		m_result = false;
		return;
	}

	// Merge hash of dependent pipeline with current pipeline hash.
	if (m_currentDependency)
		m_currentDependency->pipelineHash += pipelineHash;
}

bool PipelineDependsIncremental::waitUntilFinished()
{
#if defined(_DEBUG)
	if (m_result)
	{
		log::debug << L"Pipeline performance" << Endl;
		log::debug << IncreaseIndent;

		double totalTime = 0.0;
		for (std::map< const TypeInfo*, std::pair< int32_t, double > >::const_iterator i = m_buildDepTimes.begin(); i != m_buildDepTimes.end(); ++i)
		{
			log::debug << i->first->getName() << L" : " << int32_t(i->second.second * 1000.0) << L" ms in " << i->second.first << L" count(s)" << Endl;
			totalTime += i->second.second;
		}

		log::debug << L"Total : " << int32_t(totalTime * 1000.0) << L" ms" << Endl;
		log::debug << DecreaseIndent;
	}
#endif
	return m_result;
}

Ref< db::Database > PipelineDependsIncremental::getSourceDatabase() const
{
	return m_sourceDatabase;
}

Ref< const ISerializable > PipelineDependsIncremental::getObjectReadOnly(const Guid& instanceGuid)
{
	if (instanceGuid.isNotNull())
		return m_instanceCache->getObjectReadOnly(instanceGuid);
	else
		return nullptr;
}

Ref< File > PipelineDependsIncremental::getFile(const Path& filePath)
{
	return FileSystem::getInstance().get(filePath);
}

Ref< IStream > PipelineDependsIncremental::openFile(const Path& filePath)
{
	Ref< IStream > fileStream = FileSystem::getInstance().open(filePath, File::FmRead);
	return fileStream ? new BufferedStream(fileStream) : nullptr;
}

void PipelineDependsIncremental::addUniqueDependency(
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	const std::wstring& outputPath,
	const Guid& outputGuid,
	uint32_t flags
)
{
	const TypeInfo* pipelineType;
	uint32_t pipelineHash;

	// Find appropriate pipeline.
	if (!m_pipelineFactory->findPipelineType(type_of(sourceAsset), pipelineType, pipelineHash))
	{
		log::error << L"Unable to add dependency to \"" << outputPath << L"\"; no pipeline found." << Endl;
		m_result = false;
		return;
	}

	// Create dependency, add to "parent" dependency as well.
	Ref< PipelineDependency > dependency = new PipelineDependency();
	dependency->pipelineType = pipelineType;
	dependency->pipelineHash = pipelineHash;
	dependency->sourceInstanceGuid = sourceInstance ? sourceInstance->getGuid() : Guid();
	dependency->sourceAsset = sourceAsset;
	dependency->outputPath = outputPath;
	dependency->outputGuid = outputGuid;
	dependency->flags = flags;

	uint32_t dependencyIndex = m_dependencySet->add(outputGuid, dependency);

	if (m_currentDependency)
		m_currentDependency->children.insert(dependencyIndex);

	Ref< IPipeline > pipeline = m_pipelineFactory->findPipeline(*dependency->pipelineType);
	T_ASSERT(pipeline);

	bool result = true;

	// Recurse scan child dependencies.
	if (m_currentRecursionDepth < m_maxRecursionDepth)
	{
		T_ANONYMOUS_VAR(Save< uint32_t >)(m_currentRecursionDepth, m_currentRecursionDepth + 1);
		T_ANONYMOUS_VAR(Save< Ref< PipelineDependency > >)(m_currentDependency, dependency);

#if defined(_DEBUG)
		if (m_buildDepTimeStack.empty())
			m_timer.start();

		m_buildDepTimeStack.push_back(m_timer.getElapsedTime());
#endif

		result = pipeline->buildDependencies(
			this,
			sourceInstance,
			sourceAsset,
			outputPath,
			outputGuid
		);

#if defined(_DEBUG)
		double duration = m_timer.getElapsedTime() - m_buildDepTimeStack.back();
		m_buildDepTimes[dependency->pipelineType].first++;
		m_buildDepTimes[dependency->pipelineType].second += duration;
		m_buildDepTimeStack.pop_back();

		// Remove duration from parent build steps; not inclusive times.
		for (auto& times : m_buildDepTimeStack)
			times += duration;
#endif
	}

	if (result)
		updateDependencyHashes(dependency, pipeline, sourceInstance);
	else
	{
		dependency->flags |= PdfFailed;
		m_result = false;
	}
}

void PipelineDependsIncremental::updateDependencyHashes(
	PipelineDependency* dependency,
	const IPipeline* pipeline,
	const db::Instance* sourceInstance
)
{
	// Calculate source of source asset.
	dependency->sourceAssetHash = pipeline->hashAsset(dependency->sourceAsset);

	// Calculate hash of instance data.
	dependency->sourceDataHash = 0;
	if (sourceInstance)
	{
		std::vector< std::wstring > dataNames;
		DateTime lastWriteTime;

		sourceInstance->getDataNames(dataNames);
		for (const auto& dataName : dataNames)
		{
			std::wstring fauxDataPath = sourceInstance->getPath() + L"$" + dataName;

			if (m_pipelineDb && sourceInstance->getDataLastWriteTime(dataName, lastWriteTime))
			{
				PipelineFileHash fileHash;
				if (m_pipelineDb->getFile(fauxDataPath, fileHash))
				{
					if (fileHash.lastWriteTime == lastWriteTime)
					{
						dependency->sourceDataHash += fileHash.hash;
						continue;
					}
				}
			}

			Ref< IStream > dataStream = sourceInstance->readData(dataName);
			if (dataStream)
			{
				uint8_t buffer[4096];
				Adler32 a32;
				int64_t r;

				a32.begin();
				while ((r = dataStream->read(buffer, sizeof(buffer))) > 0)
					a32.feed(buffer, r);
				a32.end();

				dependency->sourceDataHash += a32.get();

				if (m_pipelineDb)
				{
					PipelineFileHash fileHash;
					fileHash.size = 0;
					fileHash.lastWriteTime = lastWriteTime;
					fileHash.hash = a32.get();
					m_pipelineDb->setFile(fauxDataPath, fileHash);
				}
			}
		}
	}

	// Calculate external file hashes.
	dependency->filesHash = 0;
	for (PipelineDependency::external_files_t::iterator i = dependency->files.begin(); i != dependency->files.end(); ++i)
	{
		if (m_pipelineDb)
		{
			Ref< File > file = FileSystem::getInstance().get(i->filePath);
			if (file)
			{
				PipelineFileHash fileHash;
				if (m_pipelineDb->getFile(i->filePath, fileHash))
				{
					if (fileHash.lastWriteTime == file->getLastWriteTime())
					{
						dependency->filesHash += fileHash.hash;
						continue;
					}
				}
			}
		}

		Ref< IStream > fileStream = FileSystem::getInstance().open(i->filePath, File::FmRead);
		if (fileStream)
		{
			uint8_t buffer[4096];
			Adler32 a32;
			int64_t r;

			a32.begin();
			while ((r = fileStream->read(buffer, sizeof(buffer))) > 0)
				a32.feed(buffer, r);
			a32.end();

			dependency->filesHash += a32.get();
			fileStream->close();

			if (m_pipelineDb)
			{
				Ref< File > file = FileSystem::getInstance().get(i->filePath);
				if (file)
				{
					PipelineFileHash fileHash;
					fileHash.size = file->getSize();
					fileHash.lastWriteTime = file->getLastWriteTime();
					fileHash.hash = a32.get();
					m_pipelineDb->setFile(i->filePath, fileHash);
				}
			}
		}
	}
}

	}
}
