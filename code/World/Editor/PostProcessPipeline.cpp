#include "Core/RefArray.h"
#include "Database/Instance.h"
#include "Editor/IPipelineDepends.h"
#include "Editor/IPipelineBuilder.h"
#include "World/Editor/PostProcessPipeline.h"
#include "World/PostProcess/PostProcessSettings.h"
#include "World/PostProcess/PostProcessStepBlur.h"
#include "World/PostProcess/PostProcessStepChain.h"
#include "World/PostProcess/PostProcessStepLuminance.h"
#include "World/PostProcess/PostProcessStepRepeat.h"
#include "World/PostProcess/PostProcessStepSimple.h"
#include "World/PostProcess/PostProcessStepSsao.h"
#include "World/PostProcess/PostProcessStepSmProj.h"

namespace traktor
{
	namespace world
	{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.world.PostProcessPipeline", 1, PostProcessPipeline, editor::IPipeline)

bool PostProcessPipeline::create(const editor::IPipelineSettings* settings)
{
	return true;
}

void PostProcessPipeline::destroy()
{
}

TypeInfoSet PostProcessPipeline::getAssetTypes() const
{
	TypeInfoSet typeSet;
	typeSet.insert(&type_of< PostProcessSettings >());
	return typeSet;
}

bool PostProcessPipeline::buildDependencies(
	editor::IPipelineDepends* pipelineDepends,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	Ref< const Object >& outBuildParams
) const
{
	Ref< const PostProcessSettings > postProcessSettings = checked_type_cast< const PostProcessSettings* >(sourceAsset);
	RefArray< PostProcessStep > ss;

	const RefArray< PostProcessStep >& steps = postProcessSettings->getSteps();
	for (RefArray< PostProcessStep >::const_iterator i = steps.begin(); i != steps.end(); ++i)
		ss.push_back(*i);

	while (!ss.empty())
	{
		Ref< PostProcessStep > step = ss.front(); ss.pop_front();

		if (const PostProcessStepBlur* stepBlur = dynamic_type_cast< const PostProcessStepBlur* >(step))
			pipelineDepends->addDependency(stepBlur->getShader().getGuid(), editor::PdfBuild);
		else if (const PostProcessStepChain* stepChain = dynamic_type_cast< const PostProcessStepChain* >(step))
		{
			const RefArray< PostProcessStep >& steps = stepChain->getSteps();
			for (RefArray< PostProcessStep >::const_iterator i = steps.begin(); i != steps.end(); ++i)
				ss.push_back(*i);
		}
		else if (const PostProcessStepLuminance* stepLuminance = dynamic_type_cast< const PostProcessStepLuminance* >(step))
			pipelineDepends->addDependency(stepLuminance->getShader().getGuid(), editor::PdfBuild);
		else if (const PostProcessStepRepeat* stepRepeat = dynamic_type_cast< const PostProcessStepRepeat* >(step))
			ss.push_back(stepRepeat->getStep());
		else if (const PostProcessStepSimple* stepSimple = dynamic_type_cast< const PostProcessStepSimple* >(step))
			pipelineDepends->addDependency(stepSimple->getShader().getGuid(), editor::PdfBuild);
		else if (const PostProcessStepSsao* stepSsao = dynamic_type_cast< const PostProcessStepSsao* >(step))
			pipelineDepends->addDependency(stepSsao->getShader().getGuid(), editor::PdfBuild);
		else if (const PostProcessStepSmProj* stepSmProj = dynamic_type_cast< const PostProcessStepSmProj* >(step))
			pipelineDepends->addDependency(stepSmProj->getShader().getGuid(), editor::PdfBuild);
	}

	return true;
}

bool PostProcessPipeline::buildOutput(
	editor::IPipelineBuilder* pipelineBuilder,
	const ISerializable* sourceAsset,
	uint32_t sourceAssetHash,
	const Object* buildParams,
	const std::wstring& outputPath,
	const Guid& outputGuid,
	uint32_t reason
) const
{
	Ref< db::Instance > outputInstance = pipelineBuilder->createOutputInstance(outputPath, outputGuid);
	if (!outputInstance)
		return false;

	outputInstance->setObject(sourceAsset);

	if (!outputInstance->commit())
		return false;

	return true;
}

	}
}
