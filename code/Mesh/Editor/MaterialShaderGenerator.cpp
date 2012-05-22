#include "Database/Database.h"
#include "Editor/IPipelineDepends.h"
#include "Mesh/Editor/MaterialShaderGenerator.h"
#include "Model/Material.h"
#include "Render/Resource/FragmentLinker.h"
#include "Render/Shader/External.h"
#include "Render/Shader/Nodes.h"
#include "Render/Shader/ShaderGraph.h"

namespace traktor
{
	namespace mesh
	{
		namespace
		{

const Guid c_materialShader(L"{CCDB27F2-644B-0742-857D-680E846B5BA3}");
const Guid c_tplDiffuseParams(L"{4AC7418D-FF43-FE40-ADDC-33A162636FDC}");
const Guid c_tplEmissiveParams(L"{1E35F0A7-23A9-EA49-A518-125A77BAD564}");
const Guid c_tplNormalParams(L"{77489017-FBE8-4A4F-B11A-FDE48C69E021}");
const Guid c_tplOutput(L"{6DA4BE0A-BE19-4440-9B08-FC3FD1FFECDC}");
const Guid c_tplRimParams(L"{57310F3A-FEB0-7644-B641-EC3876773470}");
const Guid c_tplReflectionParams(L"{859D3A98-EB16-0F4B-A766-34B6D65096AA}");
const Guid c_tplSpecularParams(L"{68DA66E7-1D9E-FD4C-9692-D947BEA3EBAD}");
const Guid c_tplVertexParams(L"{AEBE83FB-68D4-9D45-A672-0A8487A197CD}");
const Guid c_implDiffuseConst(L"{BA68E2CA-77EB-684E-AD2B-0CD4BC35608D}");
const Guid c_implDiffuseMap(L"{EE7D62D6-B5A8-DC48-8328-A3513B998DD4}");
const Guid c_implEmissiveConst(L"{61A41113-D9F9-964A-9D90-B7A686058A26}");
const Guid c_implNormalConst(L"{5D881AE1-B99D-8941-B949-4E95AEF1CB7A}");
const Guid c_implNormalMap(L"{8CA655BD-E17B-5A48-B6C6-3FDBC1D4F97D}");
const Guid c_implRimConst(L"{449F16EF-5C14-4940-A5E1-E1ABF73CC5D7}");
const Guid c_implReflectionConst(L"{2E6EC61A-E1C2-D545-A29A-B4CBED8914E5}");
const Guid c_implOutputAdd(L"{321B8969-32D7-D44A-BF91-B056E4728DE2}");
const Guid c_implOutputAlpha(L"{1CDA749C-D713-974F-8E84-895AFEE8D552}");
const Guid c_implOutputDecal(L"{31FD2B2B-3D3C-024F-9AA6-544B73D6009C}");
const Guid c_implOutputMultiply(L"{C635E09A-8DFD-BF40-A863-81301D2388AC}");
const Guid c_implSpecularConst(L"{6D818781-04A2-2E48-9340-BEFB493F1F9E}");
const Guid c_implSpecularMap(L"{208A6FD9-8591-294F-BEAB-B2B872992960}");
const Guid c_implVertex(L"{5CCADFD7-6421-9848-912E-205358848F37}");

class FragmentReaderAdapter : public render::FragmentLinker::FragmentReader
{
public:
	FragmentReaderAdapter(db::Database* database)
	:	m_database(database)
	{
	}

	virtual Ref< const render::ShaderGraph > read(const Guid& fragmentGuid)
	{
		return m_database->getObjectReadOnly< render::ShaderGraph >(fragmentGuid);
	}

private:
	Ref< db::Database > m_database;
};

Guid lookupTexture(const std::map< std::wstring, Guid >& textures, const std::wstring& name)
{
	std::map< std::wstring, Guid >::const_iterator i = textures.find(name);
	return i != textures.end() ? i->second : Guid();
}

		}

T_IMPLEMENT_RTTI_CLASS(L"traktor.mesh.MaterialShaderGenerator", MaterialShaderGenerator, Object)

MaterialShaderGenerator::MaterialShaderGenerator(db::Database* database)
:	m_database(database)
{
}

Ref< render::ShaderGraph > MaterialShaderGenerator::generate(const model::Material& material, const std::map< std::wstring, Guid >& textures) const
{
	Ref< render::ShaderGraph > materialShaderGraph = m_database->getObjectReadOnly< render::ShaderGraph >(c_materialShader);
	T_ASSERT (materialShaderGraph);

	Guid diffuseTexture = lookupTexture(textures, material.getDiffuseMap());
	Guid specularTexture = lookupTexture(textures, material.getSpecularMap());
	Guid normalTexture = lookupTexture(textures, material.getNormalMap());

	RefArray< render::External > externalNodes;
	materialShaderGraph->findNodesOf< render::External >(externalNodes);

	for (RefArray< render::External >::iterator i = externalNodes.begin(); i != externalNodes.end(); ++i)
	{
		const Guid& fragmentGuid = (*i)->getFragmentGuid();
		T_ASSERT (fragmentGuid.isValid());

		if (fragmentGuid == c_tplDiffuseParams)
		{
			if (diffuseTexture.isNull())
				(*i)->setFragmentGuid(c_implDiffuseConst);
			else
				(*i)->setFragmentGuid(c_implDiffuseMap);
		}
		else if (fragmentGuid == c_tplEmissiveParams)
			(*i)->setFragmentGuid(c_implEmissiveConst);
		else if (fragmentGuid == c_tplNormalParams)
		{
			if (normalTexture.isNull())
				(*i)->setFragmentGuid(c_implNormalConst);
			else
				(*i)->setFragmentGuid(c_implNormalMap);
		}
		else if (fragmentGuid == c_tplOutput)
		{
			switch (material.getBlendOperator())
			{
			default:
			case model::Material::BoDecal:
				(*i)->setFragmentGuid(c_implOutputDecal);
				break;
			case model::Material::BoAdd:
				(*i)->setFragmentGuid(c_implOutputAdd);
				break;
			case model::Material::BoMultiply:
				(*i)->setFragmentGuid(c_implOutputMultiply);
				break;
			case model::Material::BoAlpha:
				(*i)->setFragmentGuid(c_implOutputAlpha);
				break;
			}
		}
		else if (fragmentGuid == c_tplRimParams)
			(*i)->setFragmentGuid(c_implRimConst);
		else if (fragmentGuid == c_tplReflectionParams)
			(*i)->setFragmentGuid(c_implReflectionConst);
		else if (fragmentGuid == c_tplSpecularParams)
		{
			if (specularTexture.isNull())
				(*i)->setFragmentGuid(c_implSpecularConst);
			else
				(*i)->setFragmentGuid(c_implSpecularMap);
		}
		else if (fragmentGuid == c_tplVertexParams)
			(*i)->setFragmentGuid(c_implVertex);
	}

	FragmentReaderAdapter fragmentReader(m_database);
	materialShaderGraph = render::FragmentLinker(fragmentReader).resolve(materialShaderGraph, false);
	if (!materialShaderGraph)
		return 0;

	const RefArray< render::Node >& nodes = materialShaderGraph->getNodes();
	for (RefArray< render::Node >::const_iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		std::wstring comment = (*i)->getComment();
		if (comment == L"Tag_DiffuseColor")
		{
			render::Color* colorNode = checked_type_cast< render::Color* >(*i);
			colorNode->setComment(L"");
			colorNode->setColor(material.getColor());
		}
		else if (comment == L"Tag_DiffuseTerm")
		{
			render::Scalar* diffuseTermNode = checked_type_cast< render::Scalar* >(*i);
			diffuseTermNode->setComment(L"");
			diffuseTermNode->set(material.getDiffuseTerm());
		}
		else if (comment == L"Tag_DiffuseMap")
		{
			render::Texture* diffuseTextureNode = checked_type_cast< render::Texture* >(*i);
			diffuseTextureNode->setComment(L"");
			diffuseTextureNode->setExternal(diffuseTexture);
		}
		else if (comment == L"Tag_Emissive")
		{
			render::Scalar* emissiveNode = checked_type_cast< render::Scalar* >(*i);
			emissiveNode->setComment(L"");
			emissiveNode->set(material.getEmissive());
		}
		else if (comment == L"Tag_NormalMap")
		{
			render::Texture* normalTextureNode = checked_type_cast< render::Texture* >(*i);
			normalTextureNode->setComment(L"");
			normalTextureNode->setExternal(normalTexture);
		}
		else if (comment == L"Tag_Reflection")
		{
			render::Scalar* reflectionNode = checked_type_cast< render::Scalar* >(*i);
			reflectionNode->setComment(L"");
			reflectionNode->set(material.getReflection());
		}
		else if (comment == L"Tag_RimIntensity")
		{
			render::Scalar* rimIntensityNode = checked_type_cast< render::Scalar* >(*i);
			rimIntensityNode->setComment(L"");
			rimIntensityNode->set(material.getRimLightIntensity());
		}
		else if (comment == L"Tag_SpecularTerm")
		{
			render::Scalar* specularTermNode = checked_type_cast< render::Scalar* >(*i);
			specularTermNode->setComment(L"");
			specularTermNode->set(material.getSpecularTerm());
		}
		else if (comment == L"Tag_SpecularMap")
		{
			render::Texture* specularTextureNode = checked_type_cast< render::Texture* >(*i);
			specularTextureNode->setComment(L"");
			specularTextureNode->setExternal(specularTexture);
		}
		else if (comment == L"Tag_SpecularRoughness")
		{
			render::Scalar* specularRoughnessNode = checked_type_cast< render::Scalar* >(*i);
			specularRoughnessNode->setComment(L"");
			specularRoughnessNode->set(material.getSpecularRoughness());
		}
	}

	return materialShaderGraph;
}

void MaterialShaderGenerator::addDependencies(editor::IPipelineDepends* pipelineDepends)
{
	pipelineDepends->addDependency(c_materialShader, editor::PdfUse);
	pipelineDepends->addDependency(c_tplDiffuseParams, editor::PdfUse);
	pipelineDepends->addDependency(c_tplEmissiveParams, editor::PdfUse);
	pipelineDepends->addDependency(c_tplNormalParams, editor::PdfUse);
	pipelineDepends->addDependency(c_tplOutput, editor::PdfUse);
	pipelineDepends->addDependency(c_tplRimParams, editor::PdfUse);
	pipelineDepends->addDependency(c_tplReflectionParams, editor::PdfUse);
	pipelineDepends->addDependency(c_tplSpecularParams, editor::PdfUse);
	pipelineDepends->addDependency(c_tplVertexParams, editor::PdfUse);
	pipelineDepends->addDependency(c_implDiffuseConst, editor::PdfUse);
	pipelineDepends->addDependency(c_implDiffuseMap, editor::PdfUse);
	pipelineDepends->addDependency(c_implEmissiveConst, editor::PdfUse);
	pipelineDepends->addDependency(c_implNormalConst, editor::PdfUse);
	pipelineDepends->addDependency(c_implNormalMap, editor::PdfUse);
	pipelineDepends->addDependency(c_implRimConst, editor::PdfUse);
	pipelineDepends->addDependency(c_implReflectionConst, editor::PdfUse);
	pipelineDepends->addDependency(c_implOutputAdd, editor::PdfUse);
	pipelineDepends->addDependency(c_implOutputAlpha, editor::PdfUse);
	pipelineDepends->addDependency(c_implOutputDecal, editor::PdfUse);
	pipelineDepends->addDependency(c_implOutputMultiply, editor::PdfUse);
	pipelineDepends->addDependency(c_implSpecularConst, editor::PdfUse);
	pipelineDepends->addDependency(c_implSpecularMap, editor::PdfUse);
	pipelineDepends->addDependency(c_implVertex, editor::PdfUse);
}

	}
}
