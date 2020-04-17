#include "Core/Rtti/TypeInfo.h"

#if defined(T_STATIC)
#	include "Mesh/MeshClassFactory.h"
#	include "Mesh/Blend/BlendMeshResource.h"
#	include "Mesh/Indoor/IndoorMeshResource.h"
#	include "Mesh/Instance/InstanceMeshResource.h"
#	include "Mesh/Lod/AutoLodMeshResource.h"
#	include "Mesh/Partition/OctreePartitionData.h"
#	include "Mesh/Partition/PartitionMeshResource.h"
#	include "Mesh/Skinned/SkinnedMeshResource.h"
#	include "Mesh/Static/StaticMeshResource.h"
#	include "Mesh/Stream/StreamMeshResource.h"

namespace traktor
{
	namespace mesh
	{

extern "C" void __module__Traktor_Mesh()
{
	T_FORCE_LINK_REF(MeshClassFactory);
	T_FORCE_LINK_REF(BlendMeshResource);
	T_FORCE_LINK_REF(IndoorMeshResource);
	T_FORCE_LINK_REF(InstanceMeshResource);
	T_FORCE_LINK_REF(AutoLodMeshResource);
	T_FORCE_LINK_REF(OctreePartitionData);
	T_FORCE_LINK_REF(PartitionMeshResource);
	T_FORCE_LINK_REF(SkinnedMeshResource);
	T_FORCE_LINK_REF(StaticMeshResource);
	T_FORCE_LINK_REF(StreamMeshResource);
}

	}
}

#endif
