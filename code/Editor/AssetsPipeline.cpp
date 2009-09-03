#include "Editor/AssetsPipeline.h"
#include "Editor/Assets.h"
#include "Editor/IPipelineManager.h"

namespace traktor
{
	namespace editor
	{

T_IMPLEMENT_RTTI_SERIALIZABLE_CLASS(L"traktor.editor.AssetsPipeline", AssetsPipeline, IPipeline)

bool AssetsPipeline::create(const Settings* settings)
{
	return true;
}

void AssetsPipeline::destroy()
{
}

uint32_t AssetsPipeline::getVersion() const
{
	return 1;
}

TypeSet AssetsPipeline::getAssetTypes() const
{
	TypeSet typeSet;
	typeSet.insert(&type_of< Assets >());
	return typeSet;
}

bool AssetsPipeline::buildDependencies(
	IPipelineManager* pipelineManager,
	const db::Instance* sourceInstance,
	const Serializable* sourceAsset,
	Ref< const Object >& outBuildParams
) const
{
	const Assets* assets = checked_type_cast< const Assets* >(sourceAsset);
	for (std::vector< Guid >::const_iterator i = assets->m_dependencies.begin(); i != assets->m_dependencies.end(); ++i)
		pipelineManager->addDependency(*i, true);
	return true;
}

bool AssetsPipeline::buildOutput(
	IPipelineManager* pipelineManager,
	const Serializable* sourceAsset,
	uint32_t sourceAssetHash,
	const Object* buildParams,
	const std::wstring& outputPath,
	const Guid& outputGuid,
	uint32_t reason
) const
{
	return true;
}

	}
}
