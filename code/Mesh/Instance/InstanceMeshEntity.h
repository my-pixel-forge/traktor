#ifndef traktor_mesh_InstanceMeshEntity_H
#define traktor_mesh_InstanceMeshEntity_H

#include "Resource/Proxy.h"
#include "Mesh/MeshEntity.h"

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

class InstanceMesh;

class T_DLLCLASS InstanceMeshEntity : public MeshEntity
{
	T_RTTI_CLASS;

public:
	InstanceMeshEntity(const Transform& transform, const resource::Proxy< InstanceMesh >& mesh);
	
	virtual Aabb3 getBoundingBox() const;

	virtual void update(const UpdateParams& update);

	virtual bool supportTechnique(render::handle_t technique) const;

	virtual void render(
		world::WorldContext& worldContext,
		world::WorldRenderView& worldRenderView,
		world::IWorldRenderPass& worldRenderPass,
		float distance
	);

	inline resource::Proxy< InstanceMesh >& getMesh() { return m_mesh; }

private:
	friend class InstanceMeshEntityRenderer;

	mutable resource::Proxy< InstanceMesh > m_mesh;
};

	}
}

#endif	// traktor_mesh_InstanceMeshEntity_H
