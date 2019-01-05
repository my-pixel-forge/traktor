/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#include <list>
#include "Core/Io/IStream.h"
#include "Core/Log/Log.h"
#include "Core/Math/Const.h"
#include "Core/Math/Format.h"
#include "Core/Misc/String.h"
#include "Core/Misc/WildCompare.h"
#include "Core/Serialization/DeepClone.h"
#include "Core/Settings/PropertyString.h"
#include "Core/Settings/PropertyStringSet.h"
#include "Core/Settings/PropertyBoolean.h"
#include "Core/Thread/Acquire.h"
#include "Database/Database.h"
#include "Database/Group.h"
#include "Database/Instance.h"
#include "Editor/IPipelineDepends.h"
#include "Editor/IPipelineBuilder.h"
#include "Editor/IPipelineSettings.h"
#include "Mesh/IMeshResource.h"
#include "Mesh/Editor/MaterialShaderGenerator.h"
#include "Mesh/Editor/MeshAsset.h"
#include "Mesh/Editor/MeshPipeline.h"
#include "Mesh/Editor/Blend/BlendMeshConverter.h"
#include "Mesh/Editor/Indoor/IndoorMeshConverter.h"
#include "Mesh/Editor/Instance/InstanceMeshConverter.h"
#include "Mesh/Editor/Lod/AutoLodMeshConverter.h"
#include "Mesh/Editor/Partition/PartitionMeshConverter.h"
#include "Mesh/Editor/Skinned/SkinnedMeshConverter.h"
#include "Mesh/Editor/Static/StaticMeshConverter.h"
#include "Mesh/Editor/Stream/StreamMeshConverter.h"
#include "Model/Model.h"
#include "Model/ModelFormat.h"
#include "Model/Operations/BakeVertexOcclusion.h"
#include "Model/Operations/CullDistantFaces.h"
#include "Model/Operations/Transform.h"
#include "Render/Editor/IProgramCompiler.h"
#include "Render/Editor/Shader/External.h"
#include "Render/Editor/Shader/FragmentLinker.h"
#include "Render/Editor/Shader/Nodes.h"
#include "Render/Editor/Shader/ShaderGraph.h"
#include "Render/Editor/Shader/ShaderGraphHash.h"
#include "Render/Editor/Shader/ShaderGraphOptimizer.h"
#include "Render/Editor/Shader/ShaderGraphStatic.h"
#include "Render/Editor/Shader/ShaderGraphTechniques.h"
#include "Render/Editor/Shader/ShaderGraphValidator.h"

namespace traktor
{
	namespace mesh
	{
		namespace
		{

const static Guid c_guidVertexInterfaceGuid(L"{0A9BE5B4-4B45-B84A-AE16-57F6483436FC}");

class FragmentReaderAdapter : public render::FragmentLinker::IFragmentReader
{
public:
	FragmentReaderAdapter(editor::IPipelineBuilder* pipelineBuilder)
	:	m_pipelineBuilder(pipelineBuilder)
	{
	}

	virtual Ref< const render::ShaderGraph > read(const Guid& fragmentGuid) const
	{
		Ref< const render::ShaderGraph > shaderGraph = m_pipelineBuilder->getObjectReadOnly< render::ShaderGraph >(fragmentGuid);
		if (!shaderGraph)
			return nullptr;

		if (render::ShaderGraphValidator(shaderGraph).validateIntegrity())
			return shaderGraph;
		else
			return nullptr;
	}

private:
	Ref< editor::IPipelineBuilder > m_pipelineBuilder;
};

bool haveVertexColors(const model::Model& model)
{
	for (uint32_t i = 0; i < model.getVertexCount(); ++i)
	{
		if (model.getVertex(i).getColor() != model::c_InvalidIndex)
			return true;
	}
	return false;
}

Guid getVertexShaderGuid(MeshAsset::MeshType meshType)
{
	switch (meshType)
	{
	case MeshAsset::MtBlend:
		return Guid(L"{14AE48E1-723D-0944-821C-4B73AC942437}");

	case MeshAsset::MtIndoor:
		return Guid(L"{14AE48E1-723D-0944-821C-4B73AC942437}");

	case MeshAsset::MtInstance:
		return Guid(L"{A714A83F-8442-6F48-A2A7-6EFA95EB75F3}");

	case MeshAsset::MtLod:
		return Guid(L"{14AE48E1-723D-0944-821C-4B73AC942437}");

	case MeshAsset::MtPartition:
		return Guid(L"{14AE48E1-723D-0944-821C-4B73AC942437}");

	case MeshAsset::MtSkinned:
		return Guid(L"{69A3CF2E-9B63-0440-9410-70AB4AE127CE}");

	case MeshAsset::MtStatic:
		return Guid(L"{14AE48E1-723D-0944-821C-4B73AC942437}");

	case MeshAsset::MtStream:
		return Guid(L"{14AE48E1-723D-0944-821C-4B73AC942437}");

	default:
		return Guid();
	}
}

		}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.mesh.MeshPipeline", 29, MeshPipeline, editor::IPipeline)

MeshPipeline::MeshPipeline()
:	m_promoteHalf(false)
,	m_enableCustomShaders(true)
,	m_enableCustomTemplates(true)
,	m_enableBakeOcclusion(true)
,	m_editor(false)
{
}

bool MeshPipeline::create(const editor::IPipelineSettings* settings)
{
	m_assetPath = settings->getProperty< std::wstring >(L"Pipeline.AssetPath", L"");
	m_promoteHalf = settings->getProperty< bool >(L"MeshPipeline.PromoteHalf", false);
	m_enableCustomShaders = settings->getProperty< bool >(L"MeshPipeline.EnableCustomShaders", true);
	m_enableCustomTemplates = settings->getProperty< bool >(L"MeshPipeline.EnableCustomTemplates", true);
	m_enableBakeOcclusion = settings->getProperty< bool >(L"MeshPipeline.BakeOcclusion", true);
	m_includeOnlyTechniques = settings->getProperty< std::set< std::wstring > >(L"ShaderPipeline.IncludeOnlyTechniques");
	m_programCompilerTypeName = settings->getProperty< std::wstring >(L"ShaderPipeline.ProgramCompiler");
	m_editor = settings->getProperty< bool >(L"Pipeline.TargetEditor", false);
	return true;
}

void MeshPipeline::destroy()
{
	m_programCompiler = 0;
}

TypeInfoSet MeshPipeline::getAssetTypes() const
{
	TypeInfoSet typeSet;
	typeSet.insert(&type_of< MeshAsset >());
	return typeSet;
}

bool MeshPipeline::buildDependencies(
	editor::IPipelineDepends* pipelineDepends,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	const std::wstring& outputPath,
	const Guid& outputGuid
) const
{
	Ref< const MeshAsset > asset = checked_type_cast< const MeshAsset* >(sourceAsset);
	T_ASSERT (asset);

	pipelineDepends->addDependency(Path(m_assetPath), asset->getFileName().getOriginal());

	// Determine vertex shader guid.
	Guid vertexShaderGuid = getVertexShaderGuid(asset->getMeshType());
	if (!vertexShaderGuid.isValid())
	{
		log::error << L"Mesh pipeline failed; unknown mesh asset type" << Endl;
		return false;
	}
	pipelineDepends->addDependency(vertexShaderGuid, editor::PdfUse);

	// Add dependencies to generator fragments.
	MaterialShaderGenerator().addDependencies(pipelineDepends);
	
	// Add dependencies to material templates.
	if (m_enableCustomTemplates)
	{
		const std::map< std::wstring, Guid >& materialTemplates = asset->getMaterialTemplates();
		for (std::map< std::wstring, Guid >::const_iterator i = materialTemplates.begin(); i != materialTemplates.end(); ++i)
			pipelineDepends->addDependency(i->second, editor::PdfUse);
	}

	// Add dependencies to "fixed" material shaders.
	if (m_enableCustomShaders)
	{
		const std::map< std::wstring, Guid >& materialShaders = asset->getMaterialShaders();
		for (std::map< std::wstring, Guid >::const_iterator i = materialShaders.begin(); i != materialShaders.end(); ++i)
			pipelineDepends->addDependency(i->second, editor::PdfUse);
	}

	// Add dependencies to material textures.
	const std::map< std::wstring, Guid >& materialTextures = asset->getMaterialTextures();
	for (std::map< std::wstring, Guid >::const_iterator i = materialTextures.begin(); i != materialTextures.end(); ++i)
		pipelineDepends->addDependency(i->second, editor::PdfBuild | editor::PdfResource);

	pipelineDepends->addDependency< render::ShaderGraph >();
	return true;
}

bool MeshPipeline::buildOutput(
	editor::IPipelineBuilder* pipelineBuilder,
	const editor::IPipelineDependencySet* dependencySet,
	const editor::PipelineDependency* dependency,
	const db::Instance* /*sourceInstance*/,
	const ISerializable* sourceAsset,
	uint32_t /*sourceAssetHash*/,
	const std::wstring& outputPath,
	const Guid& outputGuid,
	const Object* buildParams,
	uint32_t /*reason*/
) const
{
	std::map< std::wstring, model::Material > materials;
	RefArray< model::Model > models;
	uint32_t polygonCount = 0;
	Aabb3 boundingBox;

	Ref< render::IProgramCompiler > programCompiler = getProgramCompiler();
	if (!programCompiler)
		return false;

	auto asset = mandatory_non_null_type_cast< const MeshAsset* >(sourceAsset);
	auto& materialTemplates = asset->getMaterialTemplates();
	auto& materialShaders = asset->getMaterialShaders();
	auto& materialTextures = asset->getMaterialTextures();

	// We allow models to be passed as build parameters in case models
	// are procedurally generated.
	if (buildParams)
	{
		Ref< model::Model > model = mandatory_non_null_type_cast< model::Model* >(
			const_cast< Object* >(buildParams)
		);

		if (model->getPolygonCount() == 0)
		{
			log::error << L"Mesh pipeline failed; no polygons in parameter source model" << Endl;
			return false;
		}

		models.push_back(model);
	}
	else
	{
		Ref< IStream > file = pipelineBuilder->openFile(Path(m_assetPath), asset->getFileName().getOriginal());
		if (!file)
		{
			log::error << L"Mesh pipeline failed; unable to open source model (" << asset->getFileName().getOriginal() << L")" << Endl;
			return false;
		}

		// Import source model(s); merge all materials into a single list (duplicates will be overridden).
		log::info << L"Loading model \"" << asset->getFileName().getFileName() << L"\"..." << Endl;

		Ref< model::Model > model = model::ModelFormat::readAny(file, asset->getFileName().getExtension());
		if (!model)
		{
			log::error << L"Mesh pipeline failed; unable to read source model (" << asset->getFileName().getOriginal() << L")" << Endl;
			return false;
		}

		if (model->getPolygonCount() == 0)
		{
			log::error << L"Mesh pipeline failed; no polygons in source model (" << asset->getFileName().getOriginal() << L")" << Endl;
			return false;
		}

		models.push_back(model);
	}

	if (models.empty())
	{
		log::error << L"Mesh pipeline failed; no models" << Endl;
		return false;
	}

	// Merge all materials into a single list (duplicates will be overridden).
	for (auto model : models)
	{
		// Scale model according to scale factor in asset.
		model::Transform(scale(asset->getScaleFactor(), asset->getScaleFactor(), asset->getScaleFactor())).apply(*model);

		if (m_enableBakeOcclusion && asset->getBakeOcclusion())
		{
			log::info << L"Baking occlusion..." << Endl;
			model::BakeVertexOcclusion().apply(*model);
		}

		if (asset->getCullDistantFaces())
		{
			log::info << L"Culling distant faces..." << Endl;
			const Aabb3 viewerRegion(Vector4(-40.0f, -40.0f, -40.0f), Vector4(40.0f, 40.0f, 40.0f));
			model::CullDistantFaces(viewerRegion).apply(*model);
		}

		const AlignedVector< model::Material >& modelMaterials = model->getMaterials();
		if (model->getMaterials().empty())
		{
			log::error << L"Mesh pipeline failed; no materials in source model(s)" << Endl;
			return false;
		}

		for (AlignedVector< model::Material >::const_iterator j = modelMaterials.begin(); j != modelMaterials.end(); ++j)
			materials[j->getName()] = *j;

		boundingBox.contain(model->getBoundingBox());
		polygonCount += model->getPolygonCount();
	}

	// Build materials.
	AlignedVector< render::VertexElement > vertexElements;
	uint32_t vertexElementOffset = 0;

	std::map< uint32_t, Ref< render::ShaderGraph > > materialTechniqueShaderGraphs;		//< Collection of all material technique fragments; later merged into single shader.
	std::map< std::wstring, std::list< MeshMaterialTechnique > > materialTechniqueMap;	//< Map from model material to technique fragments. ["Model material":["Default":hash0, "Depth":hash1, ...]]

	Guid vertexShaderGuid = getVertexShaderGuid(asset->getMeshType());
	T_ASSERT (vertexShaderGuid.isValid());

	Guid materialGuid = vertexShaderGuid.permutate(outputGuid);
	T_ASSERT (materialGuid.isValid());

	MaterialShaderGenerator generator;

	int32_t maxInstanceCount = 0;
	int32_t jointCount = models[0]->getJointCount();
	bool vertexColor = haveVertexColors(*models[0]);

	for (std::map< std::wstring, model::Material >::const_iterator i = materials.begin(); i != materials.end(); ++i)
	{
		Ref< const render::ShaderGraph > materialShaderGraph;

		std::map< std::wstring, Guid >::const_iterator it = materialShaders.find(i->first);
		if (
			m_enableCustomShaders &&
			it != materialShaders.end()
		)
		{
			if (it->second.isNull())
			{
				log::info << L"Material \"" << i->first << L"\" disabled; skipped" << Endl;
				continue;
			}

			materialShaderGraph = pipelineBuilder->getObjectReadOnly< render::ShaderGraph >(it->second);
			if (!materialShaderGraph)
			{
				log::error << L"Mesh pipeline failed; unable to read material shader \"" << i->first << L"\"" << Endl;
				return false;
			}
		}
		else
		{
			Guid materialTemplate;

			if (m_enableCustomTemplates)
			{
				std::map< std::wstring, Guid >::const_iterator it = materialTemplates.find(i->first);
				if (it != materialTemplates.end())
					materialTemplate = it->second;
			}

			materialShaderGraph = generator.generate(
				pipelineBuilder->getSourceDatabase(),
				i->second,
				materialTemplate,
				materialTextures,
				vertexColor
			);
			if (!materialShaderGraph)
			{
				log::error << L"Mesh pipeline failed; unable to generate material shader \"" << i->first << L"\"" << Endl;
				return false;
			}
		}

		// Set vertex fragment reference.
		RefArray< render::External > externals;
		materialShaderGraph->findNodesOf< render::External >(externals);
		for (auto external : externals)
		{
			if (external->getFragmentGuid() == c_guidVertexInterfaceGuid)
				external->setFragmentGuid(vertexShaderGuid);
		}

		// Link shader fragments.
		FragmentReaderAdapter fragmentReader(pipelineBuilder);
		materialShaderGraph = render::FragmentLinker(fragmentReader).resolve(materialShaderGraph, true);
		if (!materialShaderGraph)
		{
			log::error << L"MeshPipeline failed; unable to link shader fragments, material shader \"" << i->first << L"\"" << Endl;
			return false;
		}

		// Resolve all variables.
		materialShaderGraph = render::ShaderGraphStatic(materialShaderGraph).getVariableResolved();
		if (!materialShaderGraph)
		{
			log::error << L"MeshPipeline failed; unable to resolve variables" << Endl;
			return false;
		}

		// Extract platform permutation.
		const wchar_t* platformSignature = programCompiler->getPlatformSignature();
		T_ASSERT (platformSignature);

		materialShaderGraph = render::ShaderGraphStatic(materialShaderGraph).getPlatformPermutation(platformSignature);
		if (!materialShaderGraph)
		{
			log::error << L"MeshPipeline failed; unable to get platform permutation" << Endl;
			return false;
		}

		// Get connected permutation.
		materialShaderGraph = render::ShaderGraphStatic(materialShaderGraph).getConnectedPermutation();
		if (!materialShaderGraph)
		{
			log::error << L"MeshPipeline failed; unable to freeze connected conditionals, material shader \"" << i->first << L"\"" << Endl;
			return false;
		}

		// Freeze types, get typed permutation.
		materialShaderGraph = render::ShaderGraphStatic(materialShaderGraph).getTypePermutation();
		if (!materialShaderGraph)
		{
			log::error << L"MeshPipeline failed; unable to freeze types, material shader \"" << i->first << L"\"" << Endl;
			return false;
		}

		// Merge identical branches.
		materialShaderGraph = render::ShaderGraphOptimizer(materialShaderGraph).mergeBranches();
		if (!materialShaderGraph)
		{
			log::error << L"MeshPipeline failed; unable to merge branches, material shader \"" << i->first << L"\"" << Endl;
			return false;
		}

		// Update bone count from model.
		const RefArray< render::Node >& nodes = materialShaderGraph->getNodes();
		for (RefArray< render::Node >::const_iterator j = nodes.begin(); j != nodes.end(); ++j)
		{
			if (render::IndexedUniform* indexedUniform = dynamic_type_cast< render::IndexedUniform* >(*j))
			{
				if (indexedUniform->getParameterName() == L"Joints")
				{
					// Quantize joint count to reduce number of vertex shader permutations as it
					// will cost more than excessive parameters.
					int32_t uniformJointCount = alignUp(jointCount, 4);
					if (uniformJointCount * 2 != indexedUniform->getLength())
					{
						indexedUniform->setLength(uniformJointCount * 2);		// Each bone is represented of a quaternion and a vector thus multiply by 2.
						log::info << L"Found \"Joints\" uniform parameter in material \"" << i->first << L"\"; setting length to " << uniformJointCount * 2 << L" entries." << Endl;
					}
				}
				else if (indexedUniform->getParameterName() == L"InstanceWorld")
				{
					// Determine how many instances we can use when rendering instanced meshed
					// based on how many entries in uniform.
					if (maxInstanceCount <= 0 || maxInstanceCount > indexedUniform->getLength() / 2)
					{
						maxInstanceCount = indexedUniform->getLength() / 2;		// Length of uniform is twice of max number of instances.
						log::info << L"Found \"InstanceWorld\" uniform parameter in material \"" << i->first << L"\"; using max batch size " << maxInstanceCount << L" of instance(s)." << Endl;
					}
				}
			}
		}

		// Extract each material technique.
		std::set< std::wstring > materialTechniqueNames = render::ShaderGraphTechniques(materialShaderGraph).getNames();
		if (!m_includeOnlyTechniques.empty())
		{
			std::set< std::wstring > keepTechniqueNames;
			for (std::set< std::wstring >::const_iterator i = m_includeOnlyTechniques.begin(); i != m_includeOnlyTechniques.end(); ++i)
			{
				WildCompare wc(*i);
				for (std::set< std::wstring >::const_iterator j = materialTechniqueNames.begin(); j != materialTechniqueNames.end(); ++j)
				{
					if (wc.match(*j))
						keepTechniqueNames.insert(*j);
				}
			}
			materialTechniqueNames = keepTechniqueNames;
		}

		for (std::set< std::wstring >::iterator j = materialTechniqueNames.begin(); j != materialTechniqueNames.end(); ++j)
		{
			Ref< render::ShaderGraph > materialTechniqueShaderGraph = render::ShaderGraphTechniques(materialShaderGraph).generate(*j);

			uint32_t hash = render::ShaderGraphHash::calculate(materialTechniqueShaderGraph);
			materialTechniqueShaderGraphs[hash] = materialTechniqueShaderGraph;

			MeshMaterialTechnique mt;
			mt.worldTechnique = *j;
			mt.shaderTechnique = L"M" + toString(hash);
			mt.hash = hash;

			materialTechniqueMap[i->first].push_back(mt);
		}

		// Build vertex declaration from shader vertex inputs.
		RefArray< render::VertexInput > vertexInputNodes;
		materialShaderGraph->findNodesOf< render::VertexInput >(vertexInputNodes);
		for (RefArray< render::VertexInput >::iterator j = vertexInputNodes.begin(); j != vertexInputNodes.end(); ++j)
		{
			bool elementDeclared = false;

			render::DataType elementDataType = (*j)->getDataType();
			if (m_promoteHalf)
			{
				if (elementDataType == render::DtHalf2)
					elementDataType = render::DtFloat2;
				else if (elementDataType == render::DtHalf4)
					elementDataType = render::DtFloat4;
			}

			// Is it already added to vertex declaration?
			for (AlignedVector< render::VertexElement >::iterator k = vertexElements.begin(); k != vertexElements.end(); ++k)
			{
				if (
					(*j)->getDataUsage() == k->getDataUsage() &&
					(*j)->getIndex() == k->getIndex()
				)
				{
					if (elementDataType != k->getDataType())
						log::warning << L"Identical vertex input usage but different types (" << render::getDataTypeName(elementDataType) << L" and " << render::getDataTypeName(k->getDataType()) << L")" << Endl;
					elementDeclared = true;
					break;
				}
			}

			if (!elementDeclared)
			{
				render::VertexElement element(
					(*j)->getDataUsage(),
					elementDataType,
					vertexElementOffset,
					(*j)->getIndex()
				);
				vertexElements.push_back(element);
				vertexElementOffset += element.getSize();
			}
		}
	}

#if defined(_DEBUG)
	// Dump information about mesh.
	log::info << polygonCount << L" polygon(s)" << Endl;
	log::info << L"Bounding box (" << boundingBox.mn << L")-(" << boundingBox.mx << L")" << Endl;
	log::info << L"Material techniques" << Endl;
	log::info << IncreaseIndent;

	for (std::map< std::wstring, std::list< MeshMaterialTechnique > >::const_iterator i = materialTechniqueMap.begin(); i != materialTechniqueMap.end(); ++i)
	{
		log::info << L"\"" << i->first << L"\"" << Endl;
		log::info << IncreaseIndent;

		for (std::list< MeshMaterialTechnique >::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
		{
			log::info << L"World technique: \"" << j->worldTechnique << L"\"" << Endl;
			log::info << L"Shader technique: \"" << j->shaderTechnique << L"\"" << Endl;
		}

		log::info << DecreaseIndent;
	}

	log::info << DecreaseIndent;
#endif

	// Merge all shader technique fragments into a single material shader.
	Ref< render::ShaderGraph > materialShaderGraph = new render::ShaderGraph();
	for (std::map< uint32_t, Ref< render::ShaderGraph > >::iterator i = materialTechniqueShaderGraphs.begin(); i != materialTechniqueShaderGraphs.end(); ++i)
	{
		Ref< render::ShaderGraph > materialTechniqueShaderGraph = DeepClone(i->second).create< render::ShaderGraph >();

		const RefArray< render::Edge >& edges = materialTechniqueShaderGraph->getEdges();
		const RefArray< render::Node >& nodes = materialTechniqueShaderGraph->getNodes();

		std::wstring techniqueName = L"M" + toString(i->first);
		for (RefArray< render::Node >::const_iterator j = nodes.begin(); j != nodes.end(); ++j)
		{
			if (render::VertexOutput* vertexOutputNode = dynamic_type_cast< render::VertexOutput* >(*j))
				vertexOutputNode->setTechnique(techniqueName);
			if (render::PixelOutput* pixelOutputNode = dynamic_type_cast< render::PixelOutput* >(*j))
				pixelOutputNode->setTechnique(techniqueName);
		}

		for (RefArray< render::Node >::const_iterator j = nodes.begin(); j != nodes.end(); ++j)
			materialShaderGraph->addNode(*j);
		for (RefArray< render::Edge >::const_iterator j = edges.begin(); j != edges.end(); ++j)
			materialShaderGraph->addEdge(*j);
	}

	// Build material shader.
	std::wstring materialPath = Path(outputPath).getPathOnly() + L"/" + outputGuid.format() + L"/Shader";
	if (!pipelineBuilder->buildOutput(
		materialShaderGraph,
		materialPath,
		materialGuid
	))
	{
		log::error << L"Mesh pipeline failed; unable to build material shader" << Endl;
		return false;
	}

	// Create mesh converter.
	Ref< IMeshConverter > converter;
	switch (asset->getMeshType())
	{
	case MeshAsset::MtBlend:
		converter = new BlendMeshConverter();
		break;

	case MeshAsset::MtIndoor:
		converter = new IndoorMeshConverter();
		break;

	case MeshAsset::MtInstance:
		converter = new InstanceMeshConverter();
		break;

	case MeshAsset::MtLod:
		converter = new AutoLodMeshConverter();
		break;

	case MeshAsset::MtPartition:
		converter = new PartitionMeshConverter();
		break;

	case MeshAsset::MtSkinned:
		converter = new SkinnedMeshConverter();
		break;

	case MeshAsset::MtStatic:
		converter = new StaticMeshConverter();
		break;

	case MeshAsset::MtStream:
		converter = new StreamMeshConverter();
		break;

	default:
		log::error << L"Mesh pipeline failed; unknown mesh asset type" << Endl;
		return false;
	}

	// Create render mesh.
	Ref< IMeshResource > resource = converter->createResource();
	if (!resource)
	{
		log::error << L"Mesh pipeline failed; unable to create mesh resource" << Endl;
		return false;
	}

	log::info << L"Creating mesh resource \"" << type_name(resource) << L"\"..." << Endl;

	// Create output instance.
	Ref< db::Instance > outputInstance = pipelineBuilder->createOutputInstance(
		outputPath,
		outputGuid
	);
	if (!outputInstance)
	{
		log::error << L"Mesh pipeline failed; unable to create output instance" << Endl;
		return false;
	}

	// Open asset data stream.
	Ref< IStream > stream = outputInstance->writeData(L"Data");
	if (!stream)
	{
		log::error << L"Mesh pipeline failed; unable to create mesh data stream" << Endl;
		outputInstance->revert();
		return false;
	}

	int32_t dataSize = stream->tell();

	// Convert mesh asset.
	if (!converter->convert(
		asset,
		models,
		materialGuid,
		materialTechniqueMap,
		vertexElements,
		maxInstanceCount,
		resource,
		stream
	))
	{
		log::error << L"Mesh pipeline failed; unable to convert mesh" << Endl;
		return false;
	}

	dataSize = stream->tell() - dataSize;
	stream->close();

	// Commit resource.
	outputInstance->setObject(resource);
	if (!outputInstance->commit())
	{
		log::error << L"Mesh pipeline failed; unable to commit output instance" << Endl;
		return false;
	}

	return true;
}

Ref< ISerializable > MeshPipeline::buildOutput(
	editor::IPipelineBuilder* pipelineBuilder,
	const ISerializable* sourceAsset
) const
{
	T_FATAL_ERROR;
	return 0;
}

render::IProgramCompiler* MeshPipeline::getProgramCompiler() const
{
	T_ANONYMOUS_VAR(Acquire< Semaphore >)(m_programCompilerLock);

	if (m_programCompiler)
		return m_programCompiler;

	const TypeInfo* programCompilerType = TypeInfo::find(m_programCompilerTypeName.c_str());
	if (!programCompilerType)
	{
		log::error << L"Mesh pipeline; unable to find program compiler type \"" << m_programCompilerTypeName << L"\"" << Endl;
		return 0;
	}

	m_programCompiler = dynamic_type_cast< render::IProgramCompiler* >(programCompilerType->createInstance());
	if (!m_programCompiler)
	{
		log::error << L"Mesh pipeline; unable to instanciate program compiler \"" << m_programCompilerTypeName << L"\"" << Endl;
		return 0;
	}

	return m_programCompiler;
}

	}
}
