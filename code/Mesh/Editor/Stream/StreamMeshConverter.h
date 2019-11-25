#pragma once

#include "Mesh/Editor/IMeshConverter.h"

namespace traktor
{
	namespace mesh
	{

class StreamMeshConverter : public IMeshConverter
{
public:
	virtual Ref< IMeshResource > createResource() const override final;

	virtual bool getOperations(const MeshAsset* meshAsset, RefArray< const model::IModelOperation >& outOperations) const override final;

	virtual bool convert(
		const MeshAsset* meshAsset,
		const RefArray< model::Model >& models,
		const Guid& materialGuid,
		const std::map< std::wstring, std::list< MeshMaterialTechnique > >& materialTechniqueMap,
		const AlignedVector< render::VertexElement >& vertexElements,
		int32_t maxInstanceCount,
		IMeshResource* meshResource,
		IStream* meshResourceStream
	) const override final;
};

	}
}

