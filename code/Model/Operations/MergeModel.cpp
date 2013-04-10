#include <limits>
#include "Core/Log/Log.h"
#include "Model/Model.h"
#include "Model/Operations/MergeModel.h"

namespace traktor
{
	namespace model
	{
		namespace
		{

const Scalar c_snapDistance(0.02f);
const Scalar c_snapDistanceSqr(c_snapDistance * c_snapDistance);

		}

T_IMPLEMENT_RTTI_CLASS(L"traktor.model.MergeModel", MergeModel, IModelOperation)

MergeModel::MergeModel(const Model& sourceModel, const Transform& sourceTransform)
:	m_sourceModel(sourceModel)
,	m_sourceTransform(sourceTransform)
{
}

bool MergeModel::apply(Model& model) const
{
	std::map< uint32_t, uint32_t > materialMap;

	// Merge materials.
	const std::vector< Material >& sourceMaterials = m_sourceModel.getMaterials();
	for (uint32_t i = 0; i < sourceMaterials.size(); ++i)
		materialMap[i] = model.addUniqueMaterial(sourceMaterials[i]);

	// Merge geometry.
	const std::vector< Polygon >& sourcePolygons = m_sourceModel.getPolygons();
	const std::vector< Vertex >& sourceVertices = m_sourceModel.getVertices();

	std::vector< uint32_t > vertexMap;
	vertexMap.resize(sourceVertices.size(), c_InvalidIndex);

	for (size_t i = 0; i < sourceVertices.size(); ++i)
	{
		const Vertex& sourceVertex = sourceVertices[i];

		uint32_t position = sourceVertex.getPosition();
		if (position != c_InvalidIndex)
			position = model.addUniquePosition(m_sourceTransform * m_sourceModel.getPosition(position).xyz1());

		uint32_t color = sourceVertex.getColor();
		if (color != c_InvalidIndex)
			color = model.addUniqueColor(m_sourceModel.getColor(color));

		uint32_t normal = sourceVertex.getNormal();
		if (normal != c_InvalidIndex)
			normal = model.addUniqueNormal(m_sourceTransform * m_sourceModel.getNormal(normal).xyz0());

		uint32_t tangent = sourceVertex.getTangent();
		if (tangent != c_InvalidIndex)
			tangent = model.addUniqueNormal(m_sourceTransform * m_sourceModel.getNormal(tangent).xyz0());

		uint32_t binormal = sourceVertex.getBinormal();
		if (binormal != c_InvalidIndex)
			binormal = model.addUniqueNormal(m_sourceTransform * m_sourceModel.getNormal(binormal).xyz0());

		Vertex v;
		v.setPosition(position);
		v.setColor(color);
		v.setNormal(normal);
		v.setTangent(tangent);
		v.setBinormal(binormal);

		for (uint32_t j = 0; j < sourceVertex.getTexCoordCount(); ++j)
		{
			uint32_t texCoord = sourceVertex.getTexCoord(j);
			if (texCoord != c_InvalidIndex)
			{
				texCoord = model.addUniqueTexCoord(m_sourceModel.getTexCoord(texCoord));
				v.setTexCoord(j, texCoord);
			}
		}

		for (uint32_t j = 0; j < sourceVertex.getJointInfluenceCount(); ++j)
		{
			float influence = sourceVertex.getJointInfluence(j);
			v.setJointInfluence(j, influence);
		}

		vertexMap[i] = model.addVertex(v);
	}

	std::vector< Polygon >& mergedPolygons = model.getPolygons();
	mergedPolygons.reserve(mergedPolygons.size() + sourcePolygons.size());

	std::vector< Polygon > outputPolygons;
	outputPolygons.reserve(sourcePolygons.size());

	for (uint32_t i = 0; i < sourcePolygons.size(); ++i)
	{
		const Polygon& sourcePolygon = sourcePolygons[i];
		const std::vector< uint32_t >& sourceVertices = sourcePolygon.getVertices();

		// Remap vertices.
		std::vector< uint32_t > outputVertices(sourceVertices.size());
		for (uint32_t j = 0; j < sourceVertices.size(); ++j)
			outputVertices[j] = vertexMap[sourceVertices[j]];

		// Rotate vertices; keep vertex with lowest geometrical position first.
		uint32_t minPositionIndex = ~0UL;
		Vector4 minPosition = Vector4(
			std::numeric_limits< float >::max(),
			std::numeric_limits< float >::max(),
			std::numeric_limits< float >::max()
		);

		for (uint32_t j = 0; j < outputVertices.size(); ++j)
		{
			const Vector4& position = model.getVertexPosition(outputVertices[j]);
			if (
				position.x() < minPosition.x() &&
				position.y() < minPosition.y() &&
				position.z() < minPosition.z()
			)
			{
				minPositionIndex = j;
				minPosition = position;
			}
		}

		std::rotate(outputVertices.begin(), outputVertices.begin() + minPositionIndex, outputVertices.end());

		// Create polygon.
		Polygon outputPolygon;
		outputPolygon.setMaterial(materialMap[sourcePolygon.getMaterial()]);

		if (sourcePolygon.getNormal() != c_InvalidIndex)
		{
			uint32_t normal = model.addUniqueNormal(m_sourceTransform * m_sourceModel.getNormal(sourcePolygon.getNormal()).xyz0());
			outputPolygon.setNormal(normal);
		}

		outputPolygon.setVertices(outputVertices);

		// Check if polygon is duplicated or canceling through opposite winding.
		bool duplicate = false;

		for (uint32_t j = 0; j < mergedPolygons.size(); ++j)
		{
			const Polygon& mergedPolygon = mergedPolygons[j];

			if (mergedPolygon.getVertexCount() != outputPolygon.getVertexCount())
				continue;

			uint32_t outputVertex = outputPolygon.getVertex(0);
			uint32_t mergedVertex = mergedPolygon.getVertex(0);

			const Vector4& outputPosition = model.getVertexPosition(outputVertex);
			const Vector4& mergedPosition = model.getVertexPosition(mergedVertex);

			Scalar distance = (outputPosition - mergedPosition).xyz0().length2();
			if (distance > c_snapDistanceSqr)
				continue;

			uint32_t vertexCount = outputPolygon.getVertexCount();

			duplicate = true;

			for (uint32_t k = 1; k < vertexCount; ++k)
			{
				uint32_t outputVertex = outputPolygon.getVertex(k);
				uint32_t mergedVertex = mergedPolygon.getVertex(k);

				const Vector4& outputPosition = model.getVertexPosition(outputVertex);
				const Vector4& mergedPosition = model.getVertexPosition(mergedVertex);

				Scalar distance = (outputPosition - mergedPosition).xyz0().length2();
				if (distance > c_snapDistanceSqr)
				{
					duplicate = false;
					break;
				}
			}

			if (duplicate)
				break;

			duplicate = true;

			for (uint32_t k = 1; k < vertexCount; ++k)
			{
				uint32_t outputVertex = outputPolygon.getVertex(k);
				uint32_t mergedVertex = mergedPolygon.getVertex(vertexCount - k);

				const Vector4& outputPosition = model.getVertexPosition(outputVertex);
				const Vector4& mergedPosition = model.getVertexPosition(mergedVertex);

				Scalar distance = (outputPosition - mergedPosition).xyz0().length2();
				if (distance > c_snapDistanceSqr)
				{
					duplicate = false;
					break;
				}
			}

			if (duplicate)
			{
				mergedPolygons.erase(mergedPolygons.begin() + j);
				break;
			}
		}

		if (!duplicate)
			outputPolygons.push_back(outputPolygon);
	}

	mergedPolygons.insert(mergedPolygons.end(), outputPolygons.begin(), outputPolygons.end());
	return true;
}

	}
}
