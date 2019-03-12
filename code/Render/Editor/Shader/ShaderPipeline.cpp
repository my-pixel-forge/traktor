#include "Core/Functor/Functor.h"
#include "Core/Io/FileSystem.h"
#include "Core/Io/IStream.h"
#include "Core/Log/Log.h"
#include "Core/Math/Const.h"
#include "Core/Misc/String.h"
#include "Core/Misc/WildCompare.h"
#include "Core/Serialization/MemberComposite.h"
#include "Core/Serialization/MemberStl.h"
#include "Core/Settings/PropertyBoolean.h"
#include "Core/Settings/PropertyGroup.h"
#include "Core/Settings/PropertyInteger.h"
#include "Core/Settings/PropertyString.h"
#include "Core/Settings/PropertyStringSet.h"
#include "Core/Thread/Job.h"
#include "Core/Thread/JobManager.h"
#include "Core/Thread/Thread.h"
#include "Core/Thread/ThreadManager.h"
#include "Database/Database.h"
#include "Database/Instance.h"
#include "Editor/IPipelineBuilder.h"
#include "Editor/IPipelineDepends.h"
#include "Editor/IPipelineSettings.h"
//#include "Render/Capture/Editor/ProgramCompilerCapture.h"
#include "Render/Editor/IProgramCompiler.h"
#include "Render/Editor/Shader/Edge.h"
#include "Render/Editor/Shader/External.h"
#include "Render/Editor/Shader/FragmentLinker.h"
#include "Render/Editor/Shader/Nodes.h"
#include "Render/Editor/Shader/ShaderGraph.h"
#include "Render/Editor/Shader/ShaderPipeline.h"
#include "Render/Editor/Shader/ShaderGraphCombinations.h"
#include "Render/Editor/Shader/ShaderGraphOptimizer.h"
#include "Render/Editor/Shader/ShaderGraphStatic.h"
#include "Render/Editor/Shader/ShaderGraphTechniques.h"
#include "Render/Editor/Shader/ShaderGraphValidator.h"
#include "Render/Resource/ProgramResource.h"
#include "Render/Resource/ShaderResource.h"
#include "Xml/XmlDeserializer.h"
#include "Xml/XmlSerializer.h"

namespace traktor
{
	namespace render
	{
		namespace
		{

uint32_t getPriority(const render::ShaderGraph* shaderGraph)
{
	RefArray< render::PixelOutput > nodes;
	if (shaderGraph->findNodesOf< render::PixelOutput >(nodes) == 0)
		return 0;

	uint32_t priority = 0;
	for (RefArray< render::PixelOutput >::const_iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		if ((*i)->getPriority() != 0)
			priority |= (*i)->getPriority();
		else if ((*i)->getRenderState().blendEnable)
			priority |= RpAlphaBlend;
	}

	if (priority == 0)
		priority = RpOpaque;

	return priority;
}

class FragmentReaderAdapter : public FragmentLinker::IFragmentReader
{
public:
	FragmentReaderAdapter(editor::IPipelineCommon* pipeline)
	:	m_pipeline(pipeline)
	{
	}

	virtual Ref< const ShaderGraph > read(const Guid& fragmentGuid) const
	{
		Ref< const ShaderGraph > shaderGraph = m_pipeline->getObjectReadOnly< ShaderGraph >(fragmentGuid);
		if (!shaderGraph)
			return 0;

		if (ShaderGraphValidator(shaderGraph).validateIntegrity())
			return shaderGraph;
		else
			return 0;
	}

private:
	Ref< editor::IPipelineCommon > m_pipeline;
};

struct BuildCombinationTask : public Object
{
	std::wstring name;
	std::wstring path;
	ShaderGraphCombinations* combinations;
	uint32_t combination;
	ShaderResource* shaderResource;
	ShaderResource::Technique* shaderResourceTechnique;
	ShaderResource::Combination* shaderResourceCombination;
	render::IProgramCompiler* programCompiler;
	const PropertyGroup* compilerSettings;
	render::IProgramCompiler::Stats stats;
	bool frequentUniformsAsLinear;
	int optimize;
	bool validate;
	bool result;

	void execute()
	{
		const std::map< std::wstring, uint32_t >& parameterBits = shaderResource->getParameterBits();

		// Remap parameter mask and value for this combination as shader consist of multiple techniques.
		uint32_t mask = combinations->getCombinationMask(combination);
		uint32_t value = combinations->getCombinationValue(combination);

		std::vector< std::wstring > maskNames = combinations->getParameterNames(mask);
		std::vector< std::wstring > valueNames = combinations->getParameterNames(value);

		for (const auto& maskName : maskNames)
			shaderResourceCombination->mask |= parameterBits.find(maskName)->second;
		for (const auto& valueName : valueNames)
			shaderResourceCombination->value |= parameterBits.find(valueName)->second;

		// Generate combination shader graph.
		Ref< const ShaderGraph > combinationGraph = combinations->getCombinationShaderGraph(combination);
		T_ASSERT(combinationGraph);

		// Freeze type permutation.
		Ref< ShaderGraph > programGraph = ShaderGraphStatic(combinationGraph).getTypePermutation();
		if (!programGraph)
		{
			log::error << L"ShaderPipeline failed; unable to get type permutation of \"" << path << L"\"" << Endl;
			return;
		}

		// Constant propagation; calculate constant branches.
		programGraph = ShaderGraphStatic(programGraph).getConstantFolded();
		if (!programGraph)
		{
			log::error << L"ShaderPipeline failed; unable to perform constant folding of \"" << path << L"\"" << Endl;
			return;
		}

		// Get output state resolved.
		programGraph = ShaderGraphStatic(programGraph).getStateResolved();
		if (!programGraph)
		{
			log::error << L"ShaderPipeline failed; unable to resolve render state of \"" << path << L"\"" << Endl;
			return;
		}

		// Merge identical branches.
		programGraph = ShaderGraphOptimizer(programGraph).mergeBranches();
		if (!programGraph)
		{
			log::error << L"ShaderPipeline failed; unable to merge branches of \"" << path << L"\"" << Endl;
			return;
		}

		// Insert interpolation nodes at optimal locations.
		programGraph = ShaderGraphOptimizer(programGraph).insertInterpolators(frequentUniformsAsLinear);
		if (!programGraph)
		{
			log::error << L"ShaderPipeline failed; unable to optimize shader graph \"" << path << L"\"" << Endl;
			return;
		}

		// Create swizzle nodes in order to improve compiler optimizing.
		programGraph = ShaderGraphStatic(programGraph).getSwizzledPermutation();
		if (!programGraph)
		{
			log::error << L"ShaderPipeline failed; unable to perform swizzle optimization of \"" << path << L"\"" << Endl;
			return;
		}

		// Remove redundant swizzle patterns.
		programGraph = ShaderGraphStatic(programGraph).cleanupRedundantSwizzles();
		if (!programGraph)
		{
			log::error << L"ShaderPipeline failed; unable to cleanup redundant swizzles of \"" << path << L"\"" << Endl;
			return;
		}

		// Extract uniform initial values and add to initialization block in shader resource.
		RefArray< Uniform > uniformNodes;
		programGraph->findNodesOf< Uniform >(uniformNodes);
		for (const auto uniformNode : uniformNodes)
		{
			const OutputPin* outputPin = programGraph->findSourcePin(uniformNode->getInputPin(0));
			if (!outputPin)
				continue;

			const Node* outputNode = outputPin->getNode();
			T_ASSERT(outputNode);

			if (const Scalar* scalarNode = dynamic_type_cast< const Scalar* >(outputNode))
			{
				shaderResourceCombination->initializeUniformScalar.push_back(ShaderResource::InitializeUniformScalar(uniformNode->getParameterName(), scalarNode->get()));
			}
			else if (const Vector* vectorNode = dynamic_type_cast< const Vector* >(outputNode))
			{
				shaderResourceCombination->initializeUniformVector.push_back(ShaderResource::InitializeUniformVector(uniformNode->getParameterName(), vectorNode->get()));
			}
			else if (const Color* colorNode = dynamic_type_cast< const Color* >(outputNode))
			{
				shaderResourceCombination->initializeUniformVector.push_back(ShaderResource::InitializeUniformVector(uniformNode->getParameterName(), colorNode->getColor()));
			}
			else
			{
				log::error << L"ShaderPipeline failed; initial value of uniform must be constant" << Endl;
				return;
			}
		}

		// Replace texture nodes with uniforms; keep list of texture references in shader resource.
		RefArray< Texture > textureNodes;
		programGraph->findNodesOf< Texture >(textureNodes);
		for (const auto textureNode : textureNodes)
		{
			const Guid& textureGuid = textureNode->getExternal();
			int32_t textureIndex;

			std::vector< Guid >::iterator it = std::find(shaderResourceCombination->textures.begin(), shaderResourceCombination->textures.end(), textureGuid);
			if (it != shaderResourceCombination->textures.end())
				textureIndex = int32_t(std::distance(shaderResourceCombination->textures.begin(), it));
			else
			{
				textureIndex = int32_t(shaderResourceCombination->textures.size());
				shaderResourceCombination->textures.push_back(textureGuid);
			}

			Ref< Uniform > textureUniform = new Uniform(
				getParameterNameFromTextureReferenceIndex(textureIndex),
				textureNode->getParameterType(),
				UfOnce
			);

			const OutputPin* textureUniformOutput = textureUniform->getOutputPin(0);
			T_ASSERT(textureUniformOutput);

			const OutputPin* textureNodeOutput = textureNode->getOutputPin(0);
			T_ASSERT(textureNodeOutput);

			programGraph->rewire(textureNodeOutput, textureUniformOutput);
			programGraph->addNode(textureUniform);
		}

		// Compile shader program.
		Ref< ProgramResource > programResource = programCompiler->compile(
			programGraph,
			compilerSettings,
			path,
			optimize,
			validate,
			&stats
		);
		if (!programResource)
		{
			log::error << L"ShaderPipeline failed; unable to compile shader \"" << path << L"\"" << Endl;
			return;
		}

		// Add meta tag to indicate if shader combination is opaque.
		shaderResourceCombination->priority = getPriority(programGraph);

		shaderResourceCombination->program = programResource;
		result = true;
	}
};

		}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.render.ShaderPipeline", 83, ShaderPipeline, editor::IPipeline)

ShaderPipeline::ShaderPipeline()
:	m_frequentUniformsAsLinear(false)
,	m_optimize(4)
,	m_validate(true)
,	m_debugCompleteGraphs(false)
,	m_editor(false)
{
}

bool ShaderPipeline::create(const editor::IPipelineSettings* settings)
{
	m_programCompilerTypeName = settings->getProperty< std::wstring >(L"ShaderPipeline.ProgramCompiler");
	m_compilerSettings = settings->getProperty< PropertyGroup >(L"ShaderPipeline.ProgramCompilerSettings");
	m_includeOnlyTechniques = settings->getProperty< std::set< std::wstring > >(L"ShaderPipeline.IncludeOnlyTechniques");
	m_frequentUniformsAsLinear = settings->getProperty< bool >(L"ShaderPipeline.FrequentUniformsAsLinear", m_frequentUniformsAsLinear);
	m_optimize = settings->getProperty< int32_t >(L"ShaderPipeline.Optimize", m_optimize);
	m_validate = settings->getProperty< bool >(L"ShaderPipeline.Validate", m_validate);
	m_debugCompleteGraphs = settings->getProperty< bool >(L"ShaderPipeline.DebugCompleteGraphs", false);
	m_debugPath = settings->getProperty< std::wstring >(L"ShaderPipeline.DebugPath", L"");
	m_editor = settings->getProperty< bool >(L"Pipeline.TargetEditor", false);

	T_DEBUG(L"Using optimization level " << m_optimize << (m_validate ? L" with validation" : L" without validation"));
	return true;
}

void ShaderPipeline::destroy()
{
	m_programHints = 0;
	m_programCompiler = 0;
}

TypeInfoSet ShaderPipeline::getAssetTypes() const
{
	TypeInfoSet typeSet;
	typeSet.insert< ShaderGraph >();
	return typeSet;
}

bool ShaderPipeline::buildDependencies(
	editor::IPipelineDepends* pipelineDepends,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	const std::wstring& outputPath,
	const Guid& outputGuid
) const
{
	Ref< const ShaderGraph > shaderGraph = mandatory_non_null_type_cast< const ShaderGraph* >(sourceAsset);
	Ref< IProgramCompiler > programCompiler = getProgramCompiler();
	if (!programCompiler)
		return false;

	// Extract platform permutation.
	const wchar_t* platformSignature = programCompiler->getPlatformSignature();
	T_ASSERT(platformSignature);

	shaderGraph = ShaderGraphStatic(shaderGraph).getPlatformPermutation(platformSignature);
	if (!shaderGraph)
	{
		log::error << L"ShaderPipeline failed; unable to get platform permutation" << Endl;
		return false;
	}

	// Remove unused branches; don't want to add dependencies to lingering textures et al.
	shaderGraph = ShaderGraphOptimizer(shaderGraph).removeUnusedBranches();
	T_ASSERT(shaderGraph);

	// Add fragment dependencies.
	RefArray< External > externalNodes;
	shaderGraph->findNodesOf< External >(externalNodes);

	for (RefArray< External >::const_iterator i = externalNodes.begin(); i != externalNodes.end(); ++i)
	{
		const Guid& fragmentGuid = (*i)->getFragmentGuid();
		pipelineDepends->addDependency(fragmentGuid, editor::PdfUse);
	}

	// Add external texture dependencies.
	RefArray< Texture > textureNodes;
	shaderGraph->findNodesOf< Texture >(textureNodes);

	for (RefArray< Texture >::const_iterator i = textureNodes.begin(); i != textureNodes.end(); ++i)
	{
		const Guid& textureGuid = (*i)->getExternal();
		if (textureGuid.isNotNull())
			pipelineDepends->addDependency(textureGuid, editor::PdfBuild | editor::PdfResource);
	}

	return true;
}

bool ShaderPipeline::buildOutput(
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
) const
{
	Ref< const ShaderGraph > shaderGraph = mandatory_non_null_type_cast< const ShaderGraph* >(sourceAsset);
	Ref< IProgramCompiler > programCompiler = getProgramCompiler();
	if (!programCompiler)
		return false;

	Ref< ShaderResource > shaderResource = new ShaderResource();
	uint32_t parameterBit = 1;

	// Link shader fragments.
	FragmentReaderAdapter fragmentReader(pipelineBuilder);
	shaderGraph = FragmentLinker(fragmentReader).resolve(shaderGraph, true);
	if (!shaderGraph)
	{
		log::error << L"ShaderPipeline failed; unable to link shader fragments" << Endl;
		return false;
	}

	// Resolve all variables.
	shaderGraph = ShaderGraphStatic(shaderGraph).getVariableResolved();
	if (!shaderGraph)
	{
		log::error << L"ShaderPipeline failed; unable to resolve variables" << Endl;
		return false;
	}

	// Get connected permutation.
	shaderGraph = render::ShaderGraphStatic(shaderGraph).getConnectedPermutation();
	if (!shaderGraph)
	{
		log::error << L"ShaderPipeline failed; unable to resolve connected permutation" << Endl;
		return false;
	}

	// Extract platform permutation.
	const wchar_t* platformSignature = programCompiler->getPlatformSignature();
	T_ASSERT(platformSignature);

	shaderGraph = ShaderGraphStatic(shaderGraph).getPlatformPermutation(platformSignature);
	if (!shaderGraph)
	{
		log::error << L"ShaderPipeline failed; unable to get platform permutation" << Endl;
		return false;
	}

	// Remove unused branches from shader graph.
	shaderGraph = ShaderGraphOptimizer(shaderGraph).removeUnusedBranches();
	if (!shaderGraph)
	{
		log::error << L"ShaderPipeline failed; unable to remove unused branches" << Endl;
		return false;
	}

	RefArray< ShaderGraphCombinations > shaderGraphCombinations;
	std::vector< ShaderResource::Technique* > shaderResourceTechniques;
	RefArray< BuildCombinationTask > tasks;
	RefArray< Job > jobs;

	// Generate shader graphs from techniques and combinations.
	ShaderGraphTechniques techniques(shaderGraph);

	std::set< std::wstring > techniqueNames = techniques.getNames();
	if (!m_includeOnlyTechniques.empty())
	{
		std::set< std::wstring > keepTechniqueNames;
		for (std::set< std::wstring >::const_iterator i = m_includeOnlyTechniques.begin(); i != m_includeOnlyTechniques.end(); ++i)
		{
			WildCompare wc(*i);
			for (std::set< std::wstring >::const_iterator j = techniqueNames.begin(); j != techniqueNames.end(); ++j)
			{
				if (wc.match(*j))
					keepTechniqueNames.insert(*j);
			}
		}
		techniqueNames = keepTechniqueNames;
	}

	for (std::set< std::wstring >::iterator i = techniqueNames.begin(); i != techniqueNames.end(); ++i)
	{
		if (ThreadManager::getInstance().getCurrentThread()->stopped())
			break;

		log::info << L"Building shader technique \"" << *i << L"\"..." << Endl;
		log::info << IncreaseIndent;

		Ref< ShaderGraph > shaderGraphTechnique = techniques.generate(*i);
		T_ASSERT(shaderGraphTechnique);

		Ref< ShaderGraphCombinations > combinations = new ShaderGraphCombinations(shaderGraphTechnique);
		uint32_t combinationCount = combinations->getCombinationCount();
		shaderGraphCombinations.push_back(combinations);

		ShaderResource::Technique* shaderResourceTechnique = new ShaderResource::Technique();
		shaderResourceTechnique->name = *i;
		shaderResourceTechnique->mask = 0;
		shaderResourceTechniques.push_back(shaderResourceTechnique);

		// Map parameter name to unique bits; also build parameter mask for this technique.
		const std::vector< std::wstring >& parameterNames = combinations->getParameterNames();
		for (std::vector< std::wstring >::const_iterator j = parameterNames.begin(); j != parameterNames.end(); ++j)
		{
			if (shaderResource->m_parameterBits.find(*j) == shaderResource->m_parameterBits.end())
			{
				shaderResource->m_parameterBits.insert(std::make_pair(*j, parameterBit));
				parameterBit <<= 1;
			}
			shaderResourceTechnique->mask |= shaderResource->m_parameterBits[*j];
		}

		// Optimize and compile all combination programs.
		log::info << L"Spawning " << combinationCount << L" tasks..." << Endl;

		JobManager& jobManager = JobManager::getInstance();
		for (uint32_t combination = 0; combination < combinationCount; ++combination)
		{
			Ref< BuildCombinationTask > task = new BuildCombinationTask();
			task->name = *i;
			task->path = outputPath + L" - " + *i;
			task->combinations = combinations;
			task->combination = combination;
			task->shaderResource = shaderResource;
			task->shaderResourceTechnique = shaderResourceTechnique;
			task->shaderResourceCombination = new ShaderResource::Combination;
			task->shaderResourceCombination->mask = 0;
			task->shaderResourceCombination->value = 0;
			task->programCompiler = programCompiler;
			task->compilerSettings = m_compilerSettings;
			task->frequentUniformsAsLinear = m_frequentUniformsAsLinear;
			task->optimize = m_optimize;
			task->validate = m_validate;
			task->result = false;
			tasks.push_back(task);

#if !defined(__APPLE__) && !defined(__LINUX__)
			Ref< Job > job = jobManager.add(makeFunctor(task.ptr(), &BuildCombinationTask::execute));
			jobs.push_back(job);
#else
			task->execute();
#endif
		}

		log::info << DecreaseIndent;
	}

	log::info << L"Collecting task(s)..." << Endl;

	render::IProgramCompiler::Stats stats;
	uint32_t failed = 0;

	for (size_t i = 0; i < tasks.size(); ++i)
	{
		if (!jobs.empty())
		{
			jobs[i]->wait();
			jobs[i] = 0;
		}
		if (tasks[i]->result)
		{
			ShaderResource::Technique* technique = tasks[i]->shaderResourceTechnique;
			ShaderResource::Combination* combination = tasks[i]->shaderResourceCombination;
			technique->combinations.push_back(*combination);

			stats.vertexCost += tasks[i]->stats.vertexCost;
			stats.pixelCost += tasks[i]->stats.pixelCost;
			stats.vertexSize += tasks[i]->stats.vertexSize;
			stats.pixelSize += tasks[i]->stats.pixelSize;
		}
		else
			++failed;

		delete tasks[i]->shaderResourceCombination;
		tasks[i] = 0;
	}

	for (std::vector< ShaderResource::Technique* >::iterator i = shaderResourceTechniques.begin(); i != shaderResourceTechniques.end(); ++i)
	{
		shaderResource->m_techniques.push_back(**i);
		delete *i;
	}

	shaderResourceTechniques.resize(0);
	shaderGraphCombinations.resize(0);

	log::info << L"All task(s) collected" << Endl;

	if (ThreadManager::getInstance().getCurrentThread()->stopped())
	{
		log::info << L"ShaderPipeline aborted; pipeline cancelled." << Endl;
		return false;
	}

	if (failed)
	{
		log::error << L"ShaderPipeline failed; " << failed << L" task(s) failed" << Endl;
		return false;
	}

	// Create output instance.
	Ref< db::Instance > outputInstance = pipelineBuilder->createOutputInstance(
		outputPath,
		outputGuid
	);
	if (!outputInstance)
	{
		log::error << L"ShaderPipeline failed; unable to create output instance" << Endl;
		return false;
	}

	outputInstance->setObject(shaderResource);

	if (!outputInstance->commit())
	{
		log::error << L"ShaderPipeline failed; unable to commit output instance" << Endl;
		outputInstance->revert();
		return false;
	}

#if defined(_DEBUG)
	// Show resource information.
	{
		const std::vector< ShaderResource::Technique >& techniques = shaderResource->getTechniques();
		log::info << uint32_t(techniques.size()) << L" technique(s)" << Endl;

		for (size_t i = 0; i < techniques.size(); ++i)
		{
			log::info << uint32_t(i) << L": " << techniques[i].name << Endl;
			log::info << IncreaseIndent;

			const std::vector< ShaderResource::Combination >& combinations = techniques[i].combinations;
			log::info << uint32_t(combinations.size()) << L" combination(s)" << Endl;

			for (size_t j = 0; j < combinations.size(); ++j)
			{
				log::info << uint32_t(j) << L": " << combinations[j].mask << L"|" << combinations[j].value << Endl;
				log::info << IncreaseIndent;

				const std::vector< Guid >& textures = combinations[j].textures;
				log::info << uint32_t(textures.size()) << L" texture(s)" << Endl;

				for (size_t k = 0; k < textures.size(); ++k)
					log::info << uint32_t(k) << L": " << textures[k].format() << Endl;

				log::info << DecreaseIndent;
			}

			log::info << DecreaseIndent;
		}

		log::info << L"Estimated costs" << Endl;
		log::info << L"  VS " << stats.vertexCost << Endl;
		log::info << L"  PS " << stats.pixelCost << Endl;
	}
#endif

	return true;
}

Ref< ISerializable > ShaderPipeline::buildOutput(
	editor::IPipelineBuilder* pipelineBuilder,
	const ISerializable* sourceAsset
) const
{
	T_FATAL_ERROR;
	return 0;
}

IProgramCompiler* ShaderPipeline::getProgramCompiler() const
{
	T_ANONYMOUS_VAR(Acquire< Semaphore >)(m_programCompilerLock);

	if (m_programCompiler)
		return m_programCompiler;

	const TypeInfo* programCompilerType = TypeInfo::find(m_programCompilerTypeName.c_str());
	if (!programCompilerType)
	{
		log::error << L"Shader pipeline; unable to find program compiler type \"" << m_programCompilerTypeName << L"\"" << Endl;
		return 0;
	}

	m_programCompiler = dynamic_type_cast< IProgramCompiler* >(programCompilerType->createInstance());
	if (!m_programCompiler)
	{
		log::error << L"Shader pipeline; unable to instanciate program compiler \"" << m_programCompilerTypeName << L"\"" << Endl;
		return 0;
	}

	//if (m_editor)
	//	m_programCompiler = new ProgramCompilerCapture(m_programCompiler);

	return m_programCompiler;
}

	}
}
