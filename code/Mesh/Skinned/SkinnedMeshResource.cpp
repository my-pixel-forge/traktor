/*
 * TRAKTOR
 * Copyright (c) 2022-2024 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Core/Log/Log.h"
#include "Core/Misc/TString.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/MemberAlignedVector.h"
#include "Core/Serialization/MemberComposite.h"
#include "Core/Serialization/MemberSmallMap.h"
#include "Mesh/Skinned/SkinnedMesh.h"
#include "Mesh/Skinned/SkinnedMeshResource.h"
#include "Render/IRenderSystem.h"
#include "Render/Mesh/Mesh.h"
#include "Render/Mesh/MeshReader.h"
#include "Resource/IResourceManager.h"
#include "Resource/Member.h"

namespace traktor::mesh
{
	namespace
	{

const resource::Id< render::Shader > c_shaderUpdateSkin(L"{E520B46A-24BC-764C-A3E2-819DB57B7515}");

	}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.mesh.SkinnedMeshResource", 5, SkinnedMeshResource, MeshResource)

Ref< IMesh > SkinnedMeshResource::createMesh(
	const std::wstring& name,
	IStream* dataStream,
	resource::IResourceManager* resourceManager,
	render::IRenderSystem* renderSystem,
	render::MeshFactory* meshFactory
) const
{
	Ref< render::Mesh > mesh = render::MeshReader(meshFactory).read(dataStream);
	if (!mesh)
	{
		log::error << L"Skinned mesh create failed; unable to read mesh" << Endl;
		return nullptr;
	}

	Ref< SkinnedMesh > skinnedMesh = new SkinnedMesh();

	if (!resourceManager->bind(c_shaderUpdateSkin, skinnedMesh->m_shaderUpdateSkin))
		return nullptr;

	if (!resourceManager->bind(m_shader, skinnedMesh->m_shader))
		return nullptr;

	skinnedMesh->m_mesh = mesh;

	for (const auto& p : m_parts)
	{
		const render::handle_t worldTechnique = render::getParameterHandle(p.first);
		for (const auto& part : p.second)
		{
			SkinnedMesh::Part sp;
			sp.shaderTechnique = render::getParameterHandle(part.shaderTechnique);
			sp.meshPart = part.meshPart;
			skinnedMesh->m_parts[worldTechnique].push_back(sp);
		}
	}

	int32_t jointMaxIndex = -1;
	for (auto i = m_jointMap.begin(); i != m_jointMap.end(); ++i)
		jointMaxIndex = max< int32_t >(jointMaxIndex, i->second);

	skinnedMesh->m_jointMap = m_jointMap;
	skinnedMesh->m_jointCount = jointMaxIndex + 1;

	// Create ray tracing structures.
	if (renderSystem->supportRayTracing())
	{
		const auto& part = mesh->getParts().back();
		T_FATAL_ASSERT(part.name == L"__RT__");

		AlignedVector< render::Primitives > primitives;
		primitives.push_back(part.primitives);

		Ref< const render::IVertexLayout > vertexLayout = renderSystem->createVertexLayout({
			render::VertexElement(render::DataUsage::Position,	render::DtFloat4,	0 * 4 * sizeof(float)),
			render::VertexElement(render::DataUsage::Normal,	render::DtFloat4,	1 * 4 * sizeof(float)),
			render::VertexElement(render::DataUsage::Tangent,	render::DtFloat4,	2 * 4 * sizeof(float)),
			render::VertexElement(render::DataUsage::Binormal,	render::DtFloat4,	3 * 4 * sizeof(float)),
			render::VertexElement(render::DataUsage::Custom,	render::DtFloat4,	4 * 4 * sizeof(float)),
			render::VertexElement(render::DataUsage::Custom,	render::DtFloat4,	5 * 4 * sizeof(float), 1)
		});

		skinnedMesh->m_rtAccelerationStructure = renderSystem->createAccelerationStructure(
			mesh->getAuxBuffer(SkinnedMesh::c_fccSkinPosition),
			vertexLayout,
			mesh->getIndexBuffer(),
			mesh->getIndexType(),
			primitives
		);
		if (!skinnedMesh->m_rtAccelerationStructure)
		{
			log::error << L"Skinned mesh create failed; unable to create RT acceleration structure." << Endl;
			return nullptr;
		}
	}

#if defined(_DEBUG)
	skinnedMesh->m_name = wstombs(name);
#endif

	return skinnedMesh;
}

void SkinnedMeshResource::serialize(ISerializer& s)
{
	T_ASSERT_M(s.getVersion() >= 5, L"Incorrect version");

	MeshResource::serialize(s);

	s >> resource::Member< render::Shader >(L"shader", m_shader);
	s >> MemberSmallMap<
		std::wstring,
		parts_t,
		Member< std::wstring >,
		MemberAlignedVector< Part, MemberComposite< Part > >
	>(L"parts", m_parts);
	s >> MemberSmallMap< std::wstring, int32_t >(L"jointMap", m_jointMap);
}

void SkinnedMeshResource::Part::serialize(ISerializer& s)
{
	s >> Member< std::wstring >(L"shaderTechnique", shaderTechnique);
	s >> Member< uint32_t >(L"meshPart", meshPart);
}

}
