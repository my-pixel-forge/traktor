#include <algorithm>
#include <limits>
#include "Model/Formats/ModelFormatCollada.h"
#include "Model/Model.h"
#include "Core/Serialization/ISerializable.h"
#include "Core/Misc/String.h"
#include "Core/Misc/Split.h"
#include "Core/Log/Log.h"
#include "Xml/Document.h"
#include "Xml/Element.h"
#include "Xml/Attribute.h"

namespace traktor
{
	namespace model
	{
		namespace
		{

template < typename ValueType >
inline void parseStringToArray(const std::wstring& text, std::vector< ValueType >& outValueArray)
{
	Split< std::wstring, ValueType >::any(text, L" \n", outValueArray);
}

inline bool isReference(const std::wstring& node, const std::wstring& reference)
{
	return (reference[0] == '#') ? (reference.substr(1) == node) : false;
}

inline std::wstring dereference(const std::wstring& reference)
{
	if (reference.empty())
		return L"";
	return reference[0] == L'#' ? reference.substr(1) : reference;
}

struct FloatData
{
	bool read(Ref< xml::Element > source)
	{
		Ref< const xml::Element > floatArray = source->getSingle(L"float_array");
		if (floatArray)
		{
			id = source->getAttribute(L"id", L"")->getValue();
			parseStringToArray(floatArray->getValue(), data);
			if (Ref< xml::Element > techniqueCommon = source->getSingle(L"technique_common"))
			{
				if (Ref< const xml::Element > accessor = techniqueCommon->getSingle(L"accessor"))
				{
					stride = parseString< int >(accessor->getAttribute(L"stride", L"")->getValue());
					count = parseString< int >(accessor->getAttribute(L"count", L"")->getValue());
				}
			}
			return true;
		}
		else
			return false;

	}

	Vector4 getDataPosition(uint32_t index) const
	{
		T_ASSERT((index + 1)* stride <= data.size());
		return Vector4(
			-data[index * stride + 0],
			data[index * stride + 1],
			data[index * stride + 2],
			1.0f
			);
	}
	Vector4 getDataNormal(uint32_t index) const
	{
		T_ASSERT((index + 1) * stride <= data.size());
		return Vector4(
			-data[index * stride + 0],
			data[index * stride + 1],
			data[index * stride + 2],
			0.0f
			);
	}
	Vector2 getTexcoord(uint32_t index) const
	{
		T_ASSERT((index + 1) * stride <= data.size());
		return Vector2(
					data[index * stride + 0],
			 1.0f - data[index * stride + 1]
			);
	}
	Vector4 getDataColor(uint32_t index) const
	{
		T_ASSERT((index + 1) * stride <= data.size());
		return Vector4(
			data[index * stride + 0],
			data[index * stride + 1],
			data[index * stride + 2],
			stride == 4 ? data[index * stride + 3] : 0
			);
	}

	std::wstring id;
	std::vector< float > data;
	int stride;
	int count;
};

typedef std::pair< std::wstring, std::wstring > material_ref_t;
typedef std::pair< const FloatData*, uint32_t > source_data_info_t;

struct NameData
{
	bool read(Ref< xml::Element > source)
	{
		Ref< const xml::Element > nameArray = source->getSingle(L"Name_array");
		if (nameArray)
		{
			id = source->getAttribute(L"id", L"")->getValue();
			parseStringToArray(nameArray->getValue(), data);
			return true;
		}
		else
			return false;
	}
	std::wstring id;
	std::vector< std::wstring > data;
};

struct Input
{
	void read(const xml::Element* xmlData)
	{
		semantic = xmlData->getAttribute(L"semantic")->getValue();
		source = xmlData->getAttribute(L"source")->getValue();
		if (xmlData->getAttribute(L"offset"))
			offset = parseString< uint32_t >(xmlData->getAttribute(L"offset")->getValue());
		else
			offset = 0;
		if (xmlData->getAttribute(L"set"))
			set = parseString< uint32_t >(xmlData->getAttribute(L"set")->getValue());
		else
			set = 0;
	}
	std::wstring semantic;
	std::wstring source;
	uint32_t offset;
	uint32_t set;
};

class PolygonData
{
public:
	uint32_t getPolygonCount() const { return m_vertexCounts.size(); }

	uint32_t getVertexCount(int polygonIndex) const { return m_vertexCounts[polygonIndex]; }

	std::wstring getMaterial() const { return m_material; }

	uint32_t getAttribueIndex(int vertexIndex ,int attributeOffset) const { return m_indicies[ vertexIndex + attributeOffset ]; }

	source_data_info_t findSourceData(
		const std::wstring& semantic, 
		int set,
		const std::vector< FloatData >& sourceData,
		const std::pair< std::wstring, std::wstring >& vertexTranslation
	) const
	{
		std::wstring source;
		uint32_t offset = 0;

		for (uint32_t i = 0; i < m_inputs.size(); ++i)
		{
			if (semantic == m_inputs[i].semantic && 
				set == m_inputs[i].set)
			{
				offset = m_inputs[i].offset;
				source = m_inputs[i].source;
				if (isReference(vertexTranslation.first, source))
					source = vertexTranslation.second;
				break;
			}
		}

		for (uint32_t i = 0; i < sourceData.size(); ++i)
		{
			if (isReference(sourceData[i].id, source))
				return source_data_info_t(&sourceData[i], offset);
		}

		return source_data_info_t(0, offset);
	}

	void read(xml::Element* polyList)
	{
		m_material = polyList->getAttribute(L"material", L"")->getValue();
		uint32_t polyCount = parseString< uint32_t >(polyList->getAttribute(L"count", L"0")->getValue());

		RefArray< xml::Element > inputs;
		polyList->get(L"input", inputs);
		m_inputs.resize(inputs.size());
		uint32_t stride = 0;
		for (size_t j = 0; j < inputs.size(); ++j)
		{
			m_inputs[j].read(inputs[j]);
			stride = max(stride, m_inputs[j].offset + 1);
		}
		RefArray< xml::Element > polyIndexLists;
		polyList->get(L"p", polyIndexLists);

		if (polyList->getSingle(L"vcount"))
			parseStringToArray(
				polyList->getSingle(L"vcount")->getValue(),
				m_vertexCounts);
		else if (polyIndexLists.size() == 1)
			m_vertexCounts = std::vector< uint32_t >(polyCount, 3);
		else
		{
			m_vertexCounts = std::vector< uint32_t >(polyCount);
			uint32_t oldn = 0;
			for (size_t i = 0; i < polyCount; i++)
			{
				uint32_t n = Split< std::wstring, uint32_t >::any(polyIndexLists[i]->getValue(), L" ", m_indicies);
				n /= stride;
				m_vertexCounts[i] = n - oldn;
				oldn = n;
			}
			return;
		}
		parseStringToArray(polyList->getSingle(L"p")->getValue(), m_indicies);
		reEnumerateSets();
	}

private:
	void reEnumerateSets()
	{
		struct SetInfo
		{
			SetInfo(): min(1000),max(0), count(0) {}
			uint32_t min;
			uint32_t max;
			uint32_t count;
		};

		std::wstring source;
		uint32_t offset = 0;
		std::map< std::wstring, SetInfo > setsPerSource;

		for (uint32_t i = 0; i < m_inputs.size(); ++i)
		{
			SetInfo& setInfo = setsPerSource[m_inputs[i].semantic];
			setInfo.count++;
			setInfo.min = std::min(setInfo.min, m_inputs[i].set);
			setInfo.max = std::max(setInfo.max, m_inputs[i].set);
		}
		for (uint32_t i = 0; i < m_inputs.size(); ++i)
			m_inputs[i].set -= setsPerSource[m_inputs[i].semantic].min;
	}

private:
	std::vector< uint32_t > m_vertexCounts;
	std::vector< uint32_t > m_indicies; 
	std::vector< Input > m_inputs;
	std::wstring m_material;
};

struct VertexWeightData
{
	std::vector< uint32_t > vertexCounts;
	std::vector< uint32_t > indicies; 
	std::vector< Input > inputs;
};

void fetchVertexWeightData(VertexWeightData& vertexWeightData, xml::Element* xmlData)
{
	uint32_t count = parseString< uint32_t >(xmlData->getAttribute(L"count", L"0")->getValue());

	// read joints-weights per vertex counts
	parseStringToArray(xmlData->getSingle(L"vcount")->getValue(), vertexWeightData.vertexCounts);

	uint32_t totalVertexCount = 0;
	for (uint32_t i = 0; i < count; ++i)
		totalVertexCount += vertexWeightData.vertexCounts[i];

	// read indicies to joint-weight pairs
	parseStringToArray(
		xmlData->getSingle(L"v")->getValue(),
		vertexWeightData.indicies);

	// read the inputs (refs to joints + weights)
	RefArray< xml::Element > inputs;
	xmlData->get(L"input", inputs);
	vertexWeightData.inputs.resize(inputs.size());
	for (size_t j = 0; j < inputs.size(); ++j)
		vertexWeightData.inputs[j].read(inputs[j]);
}


struct SkinData
{
	void setJointInfluence(Vertex& vertex, int index)
	{
		uint32_t* indicies = &vertexWeightData.indicies.front();
		// Step forward to our vertex
		for (int i = 0; i < index; i++)
			indicies += vertexWeightData.vertexCounts[index];

		// run through all joints influencing our vertex
		int jointCount = vertexWeightData.vertexCounts[index];
		for (int i = 0; i < jointCount; i++)
		{	

//			uint32_t index = 
//			float influence = 
//			vertex.setBoneInfluence(index, influence);
		}
	}


	std::vector< FloatData > floatData;
	std::vector< NameData > nameData;
	std::vector< Input > jointInputs;
	VertexWeightData vertexWeightData;
//	Matrix44 bindShapeMatrix;
};

void fetchSkinData(
				xml::Element* skin,
				SkinData& skinData
				)
{
	// read sources
	RefArray< xml::Element > sources;
	skin->get(L"source", sources);			
	int nameArrayCount = 0;
	int floatArrayCount = 0;
	for (uint32_t j = 0; j < sources.size(); ++j)
	{
		if (sources[j]->getSingle(L"float_array"))
			floatArrayCount++;
		else if (sources[j]->getSingle(L"Name_array"))
			nameArrayCount++;
	}
	skinData.floatData.resize(floatArrayCount);
	floatArrayCount = 0;
	skinData.nameData.resize(nameArrayCount);
	nameArrayCount = 0;
	for (uint32_t j = 0; j < sources.size(); ++j)
	{
		if (skinData.floatData[floatArrayCount].read(sources[j]))
			floatArrayCount++;
		else if (skinData.nameData[nameArrayCount].read(sources[j]))
			nameArrayCount++;
	}
	// read joints (bindposes)
	if (Ref< xml::Element > joints = skin->getSingle(L"joints"))
	{
		RefArray< xml::Element > inputs;
		joints->get(L"input", inputs);
		skinData.jointInputs.resize(inputs.size());
		for (uint32_t k = 0; k < inputs.size(); ++k)
		{
			skinData.jointInputs[k].read(inputs[k]);
//				outModel->
		}
	}
	// read vertex weights
	Ref< xml::Element > weights = skin->getSingle(L"vertex_weights");
	fetchVertexWeightData(skinData.vertexWeightData, weights);
}

void createGeometry(
	xml::Element* mesh,
	const std::map< std::wstring, uint32_t>& materialLookUp,
	SkinData* skinData,
	Model* outModel
)
{
	std::vector< FloatData > vertexAttributeData;
	{
		RefArray< xml::Element > sources;
		mesh->get(L"source", sources);

		vertexAttributeData.resize(sources.size());
		for (uint32_t j = 0; j < sources.size(); ++j)
		{
			vertexAttributeData[j].read(sources[j]);
		}
	}

	Ref< xml::Element > vertices = mesh->getSingle(L"vertices");
	if (!vertices)
		return;

	std::pair< std::wstring, std::wstring > vertexSourceTranslation;
	vertexSourceTranslation.first = vertices->getAttribute(L"id", L"")->getValue();
	vertexSourceTranslation.second = vertices->getSingle(L"input")->getAttribute(L"source", L"")->getValue();

	// Fetch polygon data.
	std::vector< PolygonData > polygonData;
	{
		RefArray< xml::Element > polyLists;
		mesh->get(L"polylist", polyLists);

		RefArray< xml::Element > triLists;
		mesh->get(L"triangles", triLists);

		RefArray< xml::Element > polygons;
		mesh->get(L"polygons", polygons);

		polygonData.resize(polyLists.size() + triLists.size() + polygons.size());
		
		uint32_t p = 0;
		for (uint32_t j = 0; j < polyLists.size(); ++j)
			polygonData[p++].read(polyLists[j]);

		for (uint32_t j = 0; j < triLists.size(); ++j)
			polygonData[p++].read(triLists[j]);

		for (uint32_t j = 0; j < polygons.size(); ++j)
			polygonData[p++].read(polygons[j]);
	}

	for (uint32_t j = 0; j < polygonData.size(); ++j)
	{
		uint32_t materialIndex = c_InvalidIndex;
		std::map<std::wstring, uint32_t>::const_iterator it = materialLookUp.find(polygonData[j].getMaterial());
		if (it != materialLookUp.end())
			materialIndex = it->second;
		source_data_info_t vertexDataInfo = polygonData[j].findSourceData(L"VERTEX", 0, vertexAttributeData, vertexSourceTranslation);
		source_data_info_t normalDataInfo = polygonData[j].findSourceData(L"NORMAL", 0, vertexAttributeData, vertexSourceTranslation);
		source_data_info_t texcoord0DataInfo = polygonData[j].findSourceData(L"TEXCOORD", 0, vertexAttributeData, vertexSourceTranslation);
		source_data_info_t texcoord1DataInfo = polygonData[j].findSourceData(L"TEXCOORD", 1, vertexAttributeData, vertexSourceTranslation);
		source_data_info_t vcolorDataInfo = polygonData[j].findSourceData(L"COLOR", 0, vertexAttributeData, vertexSourceTranslation);
		source_data_info_t biNormalDataInfo = polygonData[j].findSourceData(L"BINORMAL", 0, vertexAttributeData, vertexSourceTranslation);
		source_data_info_t tangentDataInfo = polygonData[j].findSourceData(L"TANGENT", 0, vertexAttributeData, vertexSourceTranslation);
		source_data_info_t texBinormalDataInfo = polygonData[j].findSourceData(L"TEXBINORMAL", 0, vertexAttributeData, vertexSourceTranslation);
		source_data_info_t texTangentDataInfo = polygonData[j].findSourceData(L"TEXTANGENT", 0, vertexAttributeData, vertexSourceTranslation);

		uint32_t vertexIndexStride = max(vertexDataInfo.second, normalDataInfo.second);
		vertexIndexStride = max(vertexIndexStride, texcoord0DataInfo.second);
		vertexIndexStride = max(vertexIndexStride, texcoord1DataInfo.second);
		vertexIndexStride = max(vertexIndexStride, vcolorDataInfo.second);
		vertexIndexStride = max(vertexIndexStride, biNormalDataInfo.second);
		vertexIndexStride = max(vertexIndexStride, tangentDataInfo.second);
		vertexIndexStride = max(vertexIndexStride, texBinormalDataInfo.second);
		vertexIndexStride = max(vertexIndexStride, texTangentDataInfo.second);
		vertexIndexStride += 1;

		uint32_t indexOffset = 0;
		for (uint32_t k = 0; k < polygonData[j].getPolygonCount(); ++k)
		{
			Polygon polygon;
			polygon.setMaterial(materialIndex);

			for (uint32_t l = 0; l < polygonData[j].getVertexCount(k); ++l)
			{
				Vertex vertex;
				uint32_t vIndex = (indexOffset + l) * vertexIndexStride;
				if (vertexDataInfo.first)
				{
					const source_data_info_t& dataInfo = vertexDataInfo;
					uint32_t index = polygonData[j].getAttribueIndex(vIndex ,dataInfo.second);
					if (index != -1)
					{
						Vector4 position = dataInfo.first->getDataPosition(index);
						vertex.setPosition(outModel->addUniquePosition(position));
					}
				}

				if (normalDataInfo.first)
				{
					const source_data_info_t& dataInfo = normalDataInfo;
					uint32_t index = polygonData[j].getAttribueIndex(vIndex, dataInfo.second);
					if (index != -1)
					{
						Vector4 normal = dataInfo.first->getDataNormal(index);
						vertex.setNormal(outModel->addUniqueNormal(normal));
					}
				}

				if (biNormalDataInfo.first)
				{
					const source_data_info_t& dataInfo = biNormalDataInfo;
					uint32_t index = polygonData[j].getAttribueIndex(vIndex, dataInfo.second);
					if (index != -1)
					{
						Vector4 binormal = dataInfo.first->getDataNormal(index);
						vertex.setBinormal(outModel->addUniqueNormal(binormal));
						if (skinData)
						{
							skinData->setJointInfluence(vertex, index);
						}
					}
				}

				if (tangentDataInfo.first)
				{
					const source_data_info_t& dataInfo = tangentDataInfo;
					uint32_t index = polygonData[j].getAttribueIndex(vIndex, dataInfo.second);
					if (index!= -1)
					{
						Vector4 tangent = dataInfo.first->getDataNormal(index);
						vertex.setBinormal(outModel->addUniqueNormal(tangent));
					}
				}

				if (texcoord0DataInfo.first)
				{
					const source_data_info_t& dataInfo = texcoord0DataInfo;
					uint32_t index = polygonData[j].getAttribueIndex(vIndex, dataInfo.second);
					if (index != -1)
					{
						Vector2 texCoord = dataInfo.first->getTexcoord(index);
						vertex.setTexCoord(outModel->addUniqueTexCoord(texCoord));
					}
				}
				// Second uv set
				if (texcoord1DataInfo.first)
				{
					const source_data_info_t& dataInfo = texcoord1DataInfo;
					uint32_t index = polygonData[j].getAttribueIndex(vIndex, dataInfo.second);
					if (index != -1)
					{
						Vector2 texCoord = dataInfo.first->getTexcoord(index);
						// Unsupported!
						//vertex.setTexCoord(outModel->addUniqueTexCoord(texCoord));
					}
				}

				if (vcolorDataInfo.first)
				{
					const source_data_info_t& dataInfo = vcolorDataInfo;
					uint32_t index = polygonData[j].getAttribueIndex(vIndex, dataInfo.second);
					if (index != -1)
					{
						Vector4 vcolor = dataInfo.first->getDataColor(index);
						vertex.setColor(outModel->addColor(vcolor));
					}
				}

				polygon.addVertex(
					outModel->addUniqueVertex(vertex)
				);
			}
//			polygon.flipWinding();
			outModel->addPolygon(polygon);
			indexOffset += polygonData[j].getVertexCount(k);
		}
	}
}

void createMesh(
	xml::Element* libraryGeometries,
	xml::Element* libraryControllers,
	const RefArray< xml::Element >& instanceGeometries,
	const RefArray< xml::Element >& instanceControllers,
	const std::map< std::wstring, uint32_t >& materialRefs,
	Model* outModel
)
{
	for (size_t i = 0; i < instanceControllers.size(); ++i)
	{
		std::wstring controllerRef = instanceControllers[i]->getAttribute(L"url", L"")->getValue();

		Ref< xml::Element > skin = libraryControllers->getSingle(L"controller[@id=" + dereference(controllerRef) + L"]/skin");
		if (!skin)
			continue;
		SkinData skinData;
		fetchSkinData(skin, skinData);

		std::wstring geometryRef = skin->getAttribute(L"source", L"")->getValue();

		Ref< xml::Element > geometry = libraryGeometries->getSingle(L"geometry[@id=" + dereference(geometryRef) + L"]/mesh");
		if (!geometry)
			continue;
		createGeometry(geometry, materialRefs, &skinData, outModel);
	}

	for (size_t i = 0; i < instanceGeometries.size(); ++i)
	{
		std::wstring geometryRef = instanceGeometries[i]->getAttribute(L"url", L"")->getValue();

		Ref< xml::Element > geometry = libraryGeometries->getSingle(L"geometry[@id=" + dereference(geometryRef) + L"]/mesh");
		if (!geometry)
			continue;
/*
		Ref< xml::Element > extra = libraryGeometries->getSingle(L"geometry[@id=" + dereference(geometryRef) + L"]/extra");
		bool doubleSided = false;
		if (extra)
		{
			Ref< xml::Element > technique = extra->getSingle(L"technique");
			if (technique)
				if (technique->getSingle(L"double_sided") && technique->getSingle(L"double_sided")->getValue() == L"1")
					doubleSided = true;

		}
*/
		createGeometry(geometry, materialRefs, 0, outModel);
	}
}

		}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.model.ModelFormatCollada", 0, ModelFormatCollada, ModelFormat)

void ModelFormatCollada::getExtensions(std::wstring& outDescription, std::vector< std::wstring >& outExtensions) const
{
	outDescription = L"Collada Object";
	outExtensions.push_back(L"dae");
}

bool ModelFormatCollada::supportFormat(const Path& filePath) const
{
	return 
		compareIgnoreCase(filePath.getExtension(), L"dae") == 0;
}
/*
#include "Core/Math/Const.h"
#include "Animation/Skeleton.h"
#include "Animation/Bone.h"
namespace {
Transform parseTranslation(xml::Element* node)
{
	if (node)
	{
		std::vector< float > t;
		parseStringToArray(node->getValue(), t);
		return Transform(Vector4(t[0], t[1], t[2], 0));
	}
	else 
		return Transform::identity();
}

Transform parseRotation(xml::Element* node)
{
	if (node)
	{
		std::vector< float > t;
		parseStringToArray(node->getValue(), t);
		float angle = deg2rad( t[3]);
		return Transform(Quaternion(Vector4(t[0] * angle, t[1] * angle, t[2] * angle, 0)));
	}
	else 
		return Transform::identity();
}

void parseJoint(xml::Element* node, animation::Skeleton& skeleton, int level)
{
	int parent = int(skeleton.getBoneCount()) - 1;
	if (node->getAttribute(L"type")->getValue() == L"JOINT")
	{
		Transform t;
		Ref<xml::Element> matrix = node->getSingle(L"matrix");
		if (matrix)
		{
			Matrix44 m;
			std::vector< float> ma;
			parseStringToArray(matrix->getValue(),ma);
			m = Matrix44(ma[0], ma[1], ma[2], ma[3], 
						 ma[4], ma[5], ma[6], ma[7], 
						 ma[8], ma[9], ma[10], ma[11], 
						 ma[12], ma[13], ma[14], ma[15]);
			t = Transform(m);
		}
		else
		{
			Transform transform = parseTranslation(node->getSingle(L"translate"));
			RefArray< xml::Element > rotations;
			node->get(L"rotate", rotations);
			for (uint32_t i=0; i < rotations.size(); i++)
			{
				Transform rotation = parseRotation(rotations[i]);
				t = t * rotation;
			}
		}
		animation::Bone b;
		if (level > 0)
			b.setParent(level - 1);
		b.setName(node->getAttribute(L"name")->getValue());
		b.setOrientation(t.rotation());
		level++;
	}
	else
	{
		return;
	}
}
}
*/

Ref< Model > ModelFormatCollada::read(const Path& filePath, uint32_t importFlags) const
{
	xml::Document doc;
	
	if (!doc.loadFromFile(filePath.getPathName()))
		return 0;

	Ref< xml::Element > scene = doc.getSingle(L"scene");
	if (!scene)
		return 0;

	Ref< xml::Element > instanceVisualScene = scene->getSingle(L"instance_visual_scene");
	const std::wstring& visualSceneRef = instanceVisualScene->getAttribute(L"url", L"")->getValue();

	Ref< xml::Element > visualScene = doc.getSingle(L"library_visual_scenes/visual_scene[@id=" + dereference(visualSceneRef) + L"]");
	if (!visualScene)
		return 0;

	// Find references to materials and geometries.
	std::vector< material_ref_t > materialRefs;
	std::map< std::wstring, uint32_t> materialLookUp;
	RefArray< xml::Element > instanceGeometries;
	RefArray< xml::Element > instanceControllers;

	RefArray< xml::Element > nodes;
	visualScene->get(L"node", nodes);
	while (!nodes.empty())
	{
		Ref< xml::Element > node = nodes.back();
		nodes.pop_back();

		Ref< xml::Element > instanceGeometry = node->getSingle(L"instance_geometry");
		if (instanceGeometry)
		{
			instanceGeometries.push_back(instanceGeometry);

			RefArray< xml::Element > instanceMaterials;
			instanceGeometry->get(L"bind_material/technique_common/instance_material", instanceMaterials);

			for (RefArray< xml::Element >::iterator i = instanceMaterials.begin(); i != instanceMaterials.end(); ++i)
			{
				std::wstring symbol = (*i)->getAttribute(L"symbol", L"")->getValue();
				std::wstring target = (*i)->getAttribute(L"target", L"")->getValue();
				if (!symbol.empty() && !target.empty())
					materialRefs.push_back(material_ref_t(symbol, target));
			}
		}

		Ref< xml::Element > instanceController = node->getSingle(L"instance_controller");
		if (instanceController)
		{
			instanceControllers.push_back(instanceController);

			RefArray< xml::Element > instanceMaterials;
			instanceController->get(L"bind_material/technique_common/instance_material", instanceMaterials);

			for (RefArray< xml::Element >::iterator i = instanceMaterials.begin(); i != instanceMaterials.end(); ++i)
			{
				std::wstring symbol = (*i)->getAttribute(L"symbol", L"")->getValue();
				std::wstring target = (*i)->getAttribute(L"target", L"")->getValue();
				if (!symbol.empty() && !target.empty())
					materialRefs.push_back(material_ref_t(symbol, target));
			}
		}
		if (node->getAttribute(L"type") && node->getAttribute(L"type")->getValue() == L"JOINT")
		{
/*
			animation::Skeleton skeleton;
			parseJoint(node, skeleton, 0);
*/
		}

		node->get(L"node", nodes);
	}

	// Create model
	Ref< Model > outModel = new Model();

	for (uint32_t i = 0; i < materialRefs.size(); ++i)
	{
		std::map<std::wstring, uint32_t>::iterator it = materialLookUp.find(materialRefs[i].first);
		if (it == materialLookUp.end())
		{
			Material m;
			//m.setName(materialRefs[i].second);		// material
			m.setName(materialRefs[i].first);		// sg
			m.setDoubleSided(false);
			uint32_t materialIndex = outModel->addMaterial(m);
			materialLookUp.insert(std::make_pair(materialRefs[i].first, materialIndex));
		}
	}

	if (importFlags & IfMesh)
	{
		Ref< xml::Element > libraryGeometries = doc.getSingle(L"library_geometries");
		Ref< xml::Element > libraryControllers = doc.getSingle(L"library_controllers");

		createMesh(
			libraryGeometries,
			libraryControllers,
			instanceGeometries,
			instanceControllers,
			materialLookUp,
			outModel
		);
	}

	return outModel;
}

bool ModelFormatCollada::write(const Path& filePath, const Model* model) const
{
	return false;
}

	}
}
