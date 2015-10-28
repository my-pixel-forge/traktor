#ifndef traktor_mesh_StreamMeshComponent_H
#define traktor_mesh_StreamMeshComponent_H

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

class StreamMesh;

/*! \brief
 * \ingroup Mesh
 */
class T_DLLCLASS StreamMeshComponent : public MeshComponent
{
	T_RTTI_CLASS;

public:
	StreamMeshComponent(world::Entity* owner, bool screenSpaceCulling, const resource::Proxy< StreamMesh >& mesh);

	virtual void destroy() T_OVERRIDE T_FINAL;

	virtual Aabb3 getBoundingBox() const T_OVERRIDE T_FINAL;

	virtual void render(world::WorldContext& worldContext, world::WorldRenderView& worldRenderView, world::IWorldRenderPass& worldRenderPass, const Transform& transform) T_OVERRIDE T_FINAL;

	uint32_t getFrameCount() const;

	void setFrame(uint32_t frame);

private:
	resource::Proxy< StreamMesh > m_mesh;
	Ref< StreamMesh::Instance > m_instance;
	uint32_t m_frame;
};

	}
}

#endif	// traktor_mesh_StreamMeshComponent_H
