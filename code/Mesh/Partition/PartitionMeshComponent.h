#pragma once

#include "Mesh/MeshComponent.h"
#include "Resource/Proxy.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_MESH_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace mesh
	{

class PartitionMesh;

/*! Partition mesh component.
 * \ingroup Mesh
 */
class T_DLLCLASS PartitionMeshComponent : public MeshComponent
{
	T_RTTI_CLASS;

public:
	PartitionMeshComponent(const resource::Proxy< PartitionMesh >& mesh, bool screenSpaceCulling);

	virtual void destroy() override final;

	virtual Aabb3 getBoundingBox() const override final;

	virtual void build(const world::WorldContext& worldContext, const world::WorldRenderView& worldRenderView, const world::IWorldRenderPass& worldRenderPass) override final;

private:
	resource::Proxy< PartitionMesh > m_mesh;
};

	}
}

