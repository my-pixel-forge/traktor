#include <fbxsdk.h>
#include "Core/Io/FileSystem.h"
#include "Core/Io/IStream.h"
#include "Core/Log/Log.h"
#include "Core/Misc/String.h"
#include "Core/Misc/TString.h"
#include "Core/Thread/Acquire.h"
#include "Model/Model.h"
#include "Model/Formats/ModelFormatFbx.h"

namespace traktor
{
	namespace model
	{
		namespace
		{

Vector2 convertVector2(const KFbxVector2& v)
{
	return Vector2(
		float(v[0]),
		float(v[1])
	);
}

Vector4 convertVector4(const KFbxVector4& v)
{
	return Vector4(
		float(-v[0]),
		float( v[1]),
		float( v[2]),
		float( v[3])
	);
}

KFbxXMatrix getGeometricTransform(const KFbxNode* fbxNode)
{
	KFbxVector4 t = fbxNode->GetGeometricTranslation(KFbxNode::eSOURCE_SET);
	KFbxVector4 r = fbxNode->GetGeometricRotation(KFbxNode::eSOURCE_SET);
	KFbxVector4 s = fbxNode->GetGeometricScaling(KFbxNode::eSOURCE_SET);
	return KFbxXMatrix(t, r, s);
}

std::wstring getTextureName(const KFbxSurfaceMaterial* material, const char* fbxPropertyName)
{
	if (material && fbxPropertyName)
	{
		const KFbxProperty prop = material->FindProperty(fbxPropertyName);
		if (prop.IsValid())
		{
			int textureCount = prop.GetSrcObjectCount(KFbxTexture::ClassId);
			for (int i = 0; i < textureCount; i++)
			{
				KFbxTexture* texture = KFbxCast< KFbxTexture >(prop.GetSrcObject(KFbxTexture::ClassId, i));
				if (texture)
				{
					const Path texturePath(mbstows(texture->GetFileName()));
					const std::wstring textureName = texturePath.getFileNameNoExtension();
					return textureName;
				}
			}
		}
	}
	return std::wstring();
}

bool convertMesh(Model& outModel, KFbxScene* scene, KFbxNode* meshNode, uint32_t importFlags)
{
	int32_t vertexId = 0;

	KFbxMesh* mesh = static_cast< KFbxMesh* >(meshNode->GetNodeAttribute());
	if (!mesh)
		return false;

	// Convert materials.
	uint32_t materialBase = c_InvalidIndex;
	int32_t materialCount = 0;
	if (importFlags & ModelFormat::IfMaterials)
	{
		materialBase = outModel.getMaterials().size();
		materialCount = meshNode->GetMaterialCount();
		for (int32_t i = 0; i < materialCount; ++i)
		{
			KFbxSurfaceMaterial* material = meshNode->GetMaterial(i);
			if (!material)
				continue;

			Material mm;
			mm.setName(mbstows(material->GetName()));

			std::wstring diffuseMap = getTextureName(material, KFbxSurfaceMaterial::sDiffuse);
			if (diffuseMap.length())
				mm.setDiffuseMap(diffuseMap);
			std::wstring specularMap = getTextureName(material, KFbxSurfaceMaterial::sSpecular);
			if (specularMap.length())
				mm.setSpecularMap(specularMap);
			std::wstring normalMap = getTextureName(material, KFbxSurfaceMaterial::sNormalMap);
			if (normalMap.length())
				mm.setNormalMap(normalMap);

			if (material->GetClassId().Is(KFbxSurfaceLambert::ClassId))
			{
				KFbxSurfaceLambert* lambertMaterial = (KFbxSurfaceLambert*)material;

				KFbxPropertyDouble3 lambertDiffuse = lambertMaterial->GetDiffuseColor();
				if (lambertDiffuse.IsValid())
				{
					fbxDouble3 diffuse = lambertDiffuse.Get();
					mm.setColor(Color4ub(
						uint8_t(diffuse[0] * 255),
						uint8_t(diffuse[1] * 255),
						uint8_t(diffuse[2] * 255),
						255
					));
				}

				KFbxPropertyDouble1 lambertDiffuseFactor = lambertMaterial->GetDiffuseFactor();
				if (lambertDiffuseFactor.IsValid())
				{
					fbxDouble1 diffuseFactor = lambertDiffuseFactor.Get();
					mm.setDiffuseTerm(float(diffuseFactor));
				}

				mm.setSpecularTerm(0.0f);
			}
			else if (material->GetClassId().Is(KFbxSurfacePhong::ClassId))
			{
				KFbxSurfacePhong* phongMaterial = (KFbxSurfacePhong*)material;

				KFbxPropertyDouble3 phongDiffuse = phongMaterial->GetDiffuseColor();
				if (phongDiffuse.IsValid())
				{
					fbxDouble3 diffuse = phongDiffuse.Get();
					mm.setColor(Color4ub(
						uint8_t(diffuse[0] * 255),
						uint8_t(diffuse[1] * 255),
						uint8_t(diffuse[2] * 255),
						255
					));
				}

				KFbxPropertyDouble1 phongDiffuseFactor = phongMaterial->GetDiffuseFactor();
				if (phongDiffuseFactor.IsValid())
				{
					fbxDouble1 diffuseFactor = phongDiffuseFactor.Get();
					mm.setDiffuseTerm(float(diffuseFactor));
				}

				KFbxPropertyDouble1 phongSpecularFactor = phongMaterial->GetSpecularFactor();
				if (phongSpecularFactor.IsValid())
				{
					fbxDouble1 specularFactor = phongSpecularFactor.Get();
					mm.setSpecularTerm(float(specularFactor));
				}

				KFbxPropertyDouble1 phongShininess = phongMaterial->GetShininess();
				if (phongShininess.IsValid())
				{
					fbxDouble1 shininess = phongShininess.Get();
					mm.setSpecularRoughness(float(shininess / 16.0));
				}
			}

			outModel.addMaterial(mm);
		}
	}

	// Convert vertex positions.
	uint32_t positionBase = c_InvalidIndex;
	if (importFlags & ModelFormat::IfMeshPositions)
	{
		KFbxVector4* controlPoints = mesh->GetControlPoints();
		if (!controlPoints)
			return false;

		positionBase = outModel.getPositions().size();

		KFbxAnimEvaluator* sceneEvaluator = scene->GetEvaluator();
		KFbxXMatrix nodeTransform = sceneEvaluator->GetNodeGlobalTransform(meshNode, KTime(0));
		KFbxXMatrix geometricTransform = getGeometricTransform(meshNode);
		KFbxXMatrix globalTransform = nodeTransform * geometricTransform;

		int32_t controlPointsCount = mesh->GetControlPointsCount();

		fbxDouble3 pivot = meshNode->RotationPivot.Get();
		for (int32_t i = 0; i < controlPointsCount; ++i)
		{
			KFbxVector4 v = globalTransform.MultT(controlPoints[i]);
			outModel.addPosition(convertVector4(v - pivot).xyz1());
		}
	}

	typedef std::map< uint32_t, float > bone_influences_t;	// bone, weight
	std::vector< bone_influences_t > vertexBones;
	if (importFlags & ModelFormat::IfMeshBlendWeights)
	{
		int32_t controlPointsCount = mesh->GetControlPointsCount();
		vertexBones.resize(controlPointsCount);

		int32_t deformerCount = mesh->GetDeformerCount(KFbxDeformer::eSKIN);
		for (int32_t i = 0; i < deformerCount; ++i)
		{
			KFbxSkin* skinDeformer = (KFbxSkin*) mesh->GetDeformer(i, KFbxDeformer::eSKIN);
			if (skinDeformer)
			{
				int32_t clusterCount = skinDeformer->GetClusterCount();
				for (int32_t j = 0; j < clusterCount; ++j)
				{
					KFbxCluster* cluster = skinDeformer->GetCluster(j);
					if (cluster)
					{
						int weightCount = cluster->GetControlPointIndicesCount();
						if (weightCount)
						{
							const KFbxNode* boneNode = cluster->GetLink();
							const char* boneName = boneNode->GetName();
							uint32_t boneIndex = outModel.addBone(mbstows(boneName));
							const double* weights = cluster->GetControlPointWeights();
							const int* indices = cluster->GetControlPointIndices();
							for (int32_t k = 0; k < weightCount; ++k)
							{
								int32_t vertexIndex = indices[k];
								float boneWeight = float(weights[k]);
								vertexBones[vertexIndex].insert(std::pair< uint32_t, float >(boneIndex, boneWeight));
							}
						}
					}
				}
			}
		}
	}

	// Convert polygons.
	if (importFlags & (ModelFormat::IfMeshPolygons | ModelFormat::IfMeshVertices))
	{
		int32_t polygonCount = mesh->GetPolygonCount();
		for (int32_t i = 0; i < polygonCount; ++i)
		{
			Polygon polygon;

			// Assign material; \fixme should probably create a polygon for each layer of material.
			if (importFlags & ModelFormat::IfMaterials)
			{
				for (int32_t j = 0; j < mesh->GetLayerCount(); ++j)
				{
					KFbxLayerElementMaterial* layerMaterials = mesh->GetLayer(j)->GetMaterials();
					if (layerMaterials)
					{
						int32_t materialIndex = layerMaterials->GetIndexArray().GetAt(i);
						polygon.setMaterial(materialBase + materialIndex);
					}
				}
				// If no material found but mesh have a material defined then set first material.
				if (materialCount > 0 && polygon.getMaterial() == c_InvalidIndex)
					polygon.setMaterial(materialBase);
			}

			int32_t polygonSize = mesh->GetPolygonSize(i);
			for (int32_t j = 0; j < polygonSize; ++j, ++vertexId)
			{
				int32_t pointIndex = mesh->GetPolygonVertex(i, j);

				Vertex vertex;
				vertex.setPosition(positionBase + pointIndex);
				for (bone_influences_t::const_iterator k = vertexBones[pointIndex].begin(); k != vertexBones[pointIndex].end(); ++k)
				{
					vertex.setBoneInfluence(k->first, k->second);
				}

				for (int32_t k = 0; k < mesh->GetLayerCount(); ++k)
				{
					// Vertex colors.
					KFbxLayerElementVertexColor* layerVertexColors = mesh->GetLayer(k)->GetVertexColors();
					if (layerVertexColors)
					{
					}

					// Vertex texture UVs.
					KFbxLayerElementUV* layerUVs = mesh->GetLayer(k)->GetUVs();
					if (layerUVs)
					{
						switch (layerUVs->GetMappingMode())
						{
						case KFbxLayerElement::eBY_CONTROL_POINT:
							switch (layerUVs->GetReferenceMode())
							{
							case KFbxLayerElement::eDIRECT:
								{
									Vector2 uv = convertVector2(layerUVs->GetDirectArray().GetAt(pointIndex));
									vertex.setTexCoord(0, outModel.addUniqueTexCoord(uv));
								}
								break;

							case KFbxLayerElement::eINDEX_TO_DIRECT:
								{
									int32_t id = layerUVs->GetIndexArray().GetAt(pointIndex);
									Vector2 uv = convertVector2(layerUVs->GetDirectArray().GetAt(id));
									vertex.setTexCoord(0, outModel.addUniqueTexCoord(uv));
								}
								break;

							default:
								break;
							}
							break;

						case KFbxLayerElement::eBY_POLYGON_VERTEX:
							{
								int32_t textureUVIndex = mesh->GetTextureUVIndex(i, j);
								switch (layerUVs->GetReferenceMode())
								{
								case KFbxLayerElement::eDIRECT:
								case KFbxLayerElement::eINDEX_TO_DIRECT:
									{
										Vector2 uv = convertVector2(layerUVs->GetDirectArray().GetAt(textureUVIndex));
										vertex.setTexCoord(0, outModel.addUniqueTexCoord(uv));
									}
									break;

								default:
									break;
								}
							}
							break;
							
						default:
							break;
						}
					}

					// Vertex normals.
					KFbxLayerElementNormal* layerNormals = mesh->GetLayer(k)->GetNormals();
					if (layerNormals)
					{
						if (layerNormals->GetMappingMode() == KFbxLayerElement::eBY_POLYGON_VERTEX)
						{
							switch (layerNormals->GetReferenceMode())
							{
							case KFbxLayerElement::eDIRECT:
								{
									Vector4 normal = convertVector4(layerNormals->GetDirectArray().GetAt(vertexId)).xyz0().normalized();
									vertex.setNormal(outModel.addUniqueNormal(normal));
								}
								break;

							case KFbxLayerElement::eINDEX_TO_DIRECT:
								{
									int32_t id = layerNormals->GetIndexArray().GetAt(vertexId);
									Vector4 normal = convertVector4(layerNormals->GetDirectArray().GetAt(id)).xyz0().normalized();
									vertex.setNormal(outModel.addUniqueNormal(normal));
								}
								break;

							default:
								break;
							}
						}
					}

					// Vertex tangents.
					KFbxLayerElementTangent* layerTangents = mesh->GetLayer(k)->GetTangents();
					if (layerTangents)
					{
						if (layerTangents->GetMappingMode() == KFbxLayerElement::eBY_POLYGON_VERTEX)
						{
							switch (layerTangents->GetReferenceMode())
							{
							case KFbxLayerElement::eDIRECT:
								{
									Vector4 tangent = convertVector4(layerTangents->GetDirectArray().GetAt(vertexId)).xyz0().normalized();
									vertex.setTangent(outModel.addUniqueNormal(tangent));
								}
								break;

							case KFbxLayerElement::eINDEX_TO_DIRECT:
								{
									int32_t id = layerTangents->GetIndexArray().GetAt(vertexId);
									Vector4 tangent = convertVector4(layerTangents->GetDirectArray().GetAt(id)).xyz0().normalized();
									vertex.setTangent(outModel.addUniqueNormal(tangent));
								}
								break;

							default:
								break;
							}
						}
					}

					// Vertex binormals.
					KFbxLayerElementBinormal* layerBinormals = mesh->GetLayer(k)->GetBinormals();
					if (layerBinormals)
					{
						if (layerBinormals->GetMappingMode() == KFbxLayerElement::eBY_POLYGON_VERTEX)
						{
							switch (layerTangents->GetReferenceMode())
							{
							case KFbxLayerElement::eDIRECT:
								{
									Vector4 binormal = convertVector4(layerBinormals->GetDirectArray().GetAt(vertexId)).xyz0().normalized();
									vertex.setBinormal(outModel.addUniqueNormal(binormal));
								}
								break;

							case KFbxLayerElement::eINDEX_TO_DIRECT:
								{
									int32_t id = layerBinormals->GetIndexArray().GetAt(vertexId);
									Vector4 binormal = convertVector4(layerBinormals->GetDirectArray().GetAt(id)).xyz0().normalized();
									vertex.setBinormal(outModel.addUniqueNormal(binormal));
								}
								break;

							default:
								break;
							}
						}
					}
				}

				polygon.addVertex(outModel.addUniqueVertex(vertex));
			}

			outModel.addPolygon(polygon);
		}
	}

	return true;
}

		}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.model.ModelFormatFbx", 0, ModelFormatFbx, ModelFormat)

Semaphore ModelFormatFbx::ms_lock;

void ModelFormatFbx::getExtensions(std::wstring& outDescription, std::vector< std::wstring >& outExtensions) const
{
	outDescription = L"Autodesk FBX";
	outExtensions.push_back(L"fbx");
}

bool ModelFormatFbx::supportFormat(const Path& filePath) const
{
	return compareIgnoreCase< std::wstring >(filePath.getExtension(), L"fbx") == 0;
}

Ref< Model > ModelFormatFbx::read(const Path& filePath, uint32_t importFlags) const
{
	T_ANONYMOUS_VAR(Acquire< Semaphore >)(ms_lock);

	KFbxSdkManager* sdkManager = KFbxSdkManager::Create();
	if (!sdkManager)
	{
		log::error << L"Unable to import FBX model; failed to create FBX SDK instance" << Endl;
		return 0;
	}

	KFbxIOSettings* ios = KFbxIOSettings::Create(sdkManager, IOSROOT);
	sdkManager->SetIOSettings(ios);

	KFbxScene* scene = KFbxScene::Create(sdkManager, "");
	if (!scene)
	{
		log::error << L"Unable to import FBX model; failed to create FBX scene instance" << Endl;
		return 0;
	}

	KFbxImporter* importer = KFbxImporter::Create(sdkManager, "");
	if (!importer)
	{
		log::error << L"Unable to import FBX model; failed to create FBX importer instance" << Endl;
		return 0;
	}

	bool status = importer->Initialize(wstombs(filePath.getPathName()).c_str(), -1, sdkManager->GetIOSettings());
	if (!status)
	{
		log::error << L"Unable to import FBX model; failed to initialize FBX importer" << Endl;
		return 0;
	}

	status = importer->Import(scene);
	if (!status)
	{
		log::error << L"Unable to import FBX model; FBX importer failed" << Endl;
		return 0;
	}

	Ref< Model > model = new Model();

	KFbxNode* node = scene->GetRootNode();
	if (node)
	{
		int32_t childCount = node->GetChildCount();
		for (int32_t i = 0; i < childCount; ++i)
		{
			KFbxNode* childNode = node->GetChild(i);
			if (!childNode || !childNode->GetVisibility() || !childNode->GetNodeAttribute())
				continue;

			KFbxNodeAttribute::EAttributeType attributeType = childNode->GetNodeAttribute()->GetAttributeType();
			if (attributeType == KFbxNodeAttribute::eMESH)
			{
				if (!convertMesh(*model, scene, childNode, importFlags))
				{
					log::error << L"Unable to import FBX model; failed to convert mesh" << Endl;
					return 0;
				}
			}
		}
	}

	// Create and assign default material if anonomous faces has been created.
	if (importFlags & IfMaterials)
	{
		uint32_t defaultMaterialIndex = c_InvalidIndex;
		for (uint32_t i = 0; i < model->getPolygonCount(); ++i)
		{
			const Polygon& polygon = model->getPolygon(i);
			if (polygon.getMaterial() == c_InvalidIndex)
			{
				if (defaultMaterialIndex == c_InvalidIndex)
				{
					Material material;
					material.setName(L"FBX_Default");
					material.setColor(Color4ub(255, 255, 255));
					material.setDiffuseTerm(1.0f);
					material.setSpecularTerm(1.0f);
					defaultMaterialIndex = model->addMaterial(material);
				}

				Polygon replacement = polygon;
				replacement.setMaterial(defaultMaterialIndex);
				model->setPolygon(i, replacement);
			}
		}
	}

	importer->Destroy();
	sdkManager->Destroy();

	return model;
}

bool ModelFormatFbx::write(const Path& filePath, const Model* model) const
{
	return false;
}

	}
}
