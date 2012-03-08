#ifndef traktor_mesh_MeshEntityData_H
#define traktor_mesh_MeshEntityData_H

#include "Core/Math/Matrix44.h"
#include "Mesh/AbstractMeshEntityData.h"
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

class IMesh;

class T_DLLCLASS MeshEntityData : public AbstractMeshEntityData
{
	T_RTTI_CLASS;

public:
	void setMesh(const resource::Proxy< IMesh >& mesh);

	const resource::Proxy< IMesh >& getMesh() const;

	virtual Ref< MeshEntity > createEntity(resource::IResourceManager* resourceManager, world::IEntityBuilder* builder) const;

	virtual bool serialize(ISerializer& s);

private:
	mutable resource::Proxy< IMesh > m_mesh;
};

	}
}

#endif	// traktor_mesh_MeshEntityData_H
