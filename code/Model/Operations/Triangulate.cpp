#include "Core/Math/Triangulator.h"
#include "Core/Math/Winding3.h"
#include "Model/Model.h"
#include "Model/Operations/Triangulate.h"

namespace traktor
{
	namespace model
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.model.Triangulate", Triangulate, IModelOperation)

bool Triangulate::apply(Model& model) const
{
	AlignedVector< Polygon > triangulatedPolygons;
	AlignedVector< Triangulator::Triangle > triangles;

	const auto& polygons = model.getPolygons();
	triangulatedPolygons.reserve(polygons.size());

	for (const auto& polygon : polygons)
	{
		const auto& vertices = polygon.getVertices();
		if (vertices.size() > 3)
		{
			Winding3 polygonWinding;

			for (size_t vertex : vertices)
				polygonWinding.push(model.getVertexPosition(vertex));

			Vector4 polygonNormal;
			if (polygon.getNormal() != c_InvalidIndex)
				polygonNormal = model.getNormal(polygon.getNormal());
			else
			{
				// No normal associated with polygon; try to determine from winding.
				Plane polygonPlane;
				if (!polygonWinding.getPlane(polygonPlane))
					continue;

				polygonNormal = polygonPlane.normal();
			}

			triangles.resize(0);
			Triangulator().freeze(
				polygonWinding.get(),
				polygonNormal,
				triangles
			);

			for (const auto& triangle : triangles)
			{
				Polygon triangulatedPolygon(
					polygon.getMaterial(),
					vertices[triangle.indices[0]],
					vertices[triangle.indices[1]],
					vertices[triangle.indices[2]]
				);
				triangulatedPolygons.push_back(triangulatedPolygon);
			}
		}
		else if (vertices.size() == 3)
			triangulatedPolygons.push_back(polygon);
	}

	model.setPolygons(triangulatedPolygons);
	return true;
}

void Triangulate::serialize(ISerializer& s)
{
}

	}
}
