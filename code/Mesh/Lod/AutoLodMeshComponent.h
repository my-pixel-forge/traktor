/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_mesh_AutoLodMeshComponent_H
#define traktor_mesh_AutoLodMeshComponent_H

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

class AutoLodMesh;

/*! \brief
 * \ingroup Mesh
 */
class T_DLLCLASS AutoLodMeshComponent : public MeshComponent
{
	T_RTTI_CLASS;

public:
	AutoLodMeshComponent(const resource::Proxy< AutoLodMesh >& mesh, bool screenSpaceCulling);

	virtual void destroy() override final;

	virtual Aabb3 getBoundingBox() const override final;

	virtual void render(world::WorldContext& worldContext, world::WorldRenderView& worldRenderView, world::IWorldRenderPass& worldRenderPass) override final;

private:
	resource::Proxy< AutoLodMesh > m_mesh;
	float m_lodDistance;
};

	}
}

#endif	// traktor_mesh_AutoLodMeshComponent_H
