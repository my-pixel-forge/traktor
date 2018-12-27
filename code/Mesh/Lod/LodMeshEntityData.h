/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_mesh_LodMeshEntityData_H
#define traktor_mesh_LodMeshEntityData_H

#include "Core/RefArray.h"
#include "Mesh/AbstractMeshEntityData.h"

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

class T_DLLCLASS LodMeshEntityData : public AbstractMeshEntityData
{
	T_RTTI_CLASS;

public:
	LodMeshEntityData();

	virtual Ref< MeshEntity > createEntity(resource::IResourceManager* resourceManager, const world::IEntityBuilder* builder) const override final;

	virtual void serialize(ISerializer& s) override final;
	
	virtual void setTransform(const Transform& transform) override final;

	const RefArray< AbstractMeshEntityData >& getLods() const { return m_lods; }

private:
	RefArray< AbstractMeshEntityData > m_lods;
	float m_lodDistance;
	float m_lodCullDistance;
};

	}
}

#endif	// traktor_mesh_LodMeshEntityData_H
