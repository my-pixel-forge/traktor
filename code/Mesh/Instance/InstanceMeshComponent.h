#ifndef traktor_mesh_InstanceMeshComponent_H
#define traktor_mesh_InstanceMeshComponent_H

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

class InstanceMesh;

/*! \brief
 * \ingroup Mesh
 */
class T_DLLCLASS InstanceMeshComponent : public MeshComponent
{
	T_RTTI_CLASS;

public:
	InstanceMeshComponent(world::Entity* owner, bool screenSpaceCulling, const resource::Proxy< InstanceMesh >& mesh);

	virtual void destroy() T_OVERRIDE T_FINAL;

	virtual Aabb3 getBoundingBox() const T_OVERRIDE T_FINAL;

	virtual void render(world::WorldContext& worldContext, world::WorldRenderView& worldRenderView, world::IWorldRenderPass& worldRenderPass, const Transform& transform) T_OVERRIDE T_FINAL;

private:
	resource::Proxy< InstanceMesh > m_mesh;
};

	}
}

#endif	// traktor_mesh_InstanceMeshComponent_H
