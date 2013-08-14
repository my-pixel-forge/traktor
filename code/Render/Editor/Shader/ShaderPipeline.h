#ifndef traktor_render_ShaderPipeline_H
#define traktor_render_ShaderPipeline_H

#include "Editor/IPipeline.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_RENDER_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace render
	{

class IProgramCompiler;
class IProgramHints;

class T_DLLCLASS ShaderPipeline : public editor::IPipeline
{
	T_RTTI_CLASS;

public:
	ShaderPipeline();

	virtual bool create(const editor::IPipelineSettings* settings);

	virtual void destroy();

	virtual TypeInfoSet getAssetTypes() const;

	virtual bool buildDependencies(
		editor::IPipelineDepends* pipelineDepends,
		const db::Instance* sourceInstance,
		const ISerializable* sourceAsset,
		const std::wstring& outputPath,
		const Guid& outputGuid
	) const;

	virtual bool buildOutput(
		editor::IPipelineBuilder* pipelineBuilder,
		const editor::IPipelineDependencySet* dependencySet,
		const editor::PipelineDependency* dependency,
		const db::Instance* sourceInstance,
		const ISerializable* sourceAsset,
		uint32_t sourceAssetHash,
		const std::wstring& outputPath,
		const Guid& outputGuid,
		const Object* buildParams,
		uint32_t reason
	) const;

	virtual Ref< ISerializable > buildOutput(
		editor::IPipelineBuilder* pipelineBuilder,
		const ISerializable* sourceAsset
	) const;

private:
	Ref< IProgramCompiler > m_programCompiler;
	Ref< IProgramHints > m_programHints;
	std::set< std::wstring > m_includeOnlyTechniques;
	bool m_frequentUniformsAsLinear;
	int32_t m_optimize;
	bool m_validate;
	bool m_debugCompleteGraphs;
	std::wstring m_debugPath;
};

	}
}

#endif	// traktor_render_ShaderPipeline_H
