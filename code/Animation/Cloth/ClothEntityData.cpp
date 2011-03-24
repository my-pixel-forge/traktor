#include "Animation/Cloth/ClothEntity.h"
#include "Animation/Cloth/ClothEntityData.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberStl.h"
#include "Core/Serialization/MemberComposite.h"
#include "Render/Shader/ShaderGraph.h"
#include "Resource/Member.h"

namespace traktor
{
	namespace animation
	{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"traktor.animation.ClothEntityData", 0, ClothEntityData, world::SpatialEntityData)

ClothEntityData::ClothEntityData()
:	m_resolutionX(10)
,	m_resolutionY(10)
,	m_scale(1.0f)
,	m_damping(0.01f)
,	m_solverIterations(4)
{
}

Ref< ClothEntity > ClothEntityData::createEntity(
	world::IEntityBuilder* builder,
	resource::IResourceManager* resourceManager,
	render::IRenderSystem* renderSystem,
	physics::PhysicsManager* physicsManager
) const
{
	if (!resourceManager->bind(m_shader))
		return 0;

	Ref< ClothEntity > clothEntity = new ClothEntity(physicsManager);
	if (!clothEntity->create(
		renderSystem,
		m_shader,
		m_resolutionX,
		m_resolutionY,
		m_scale,
		m_damping,
		m_solverIterations
	))
		return 0;

	// Fixate nodes by setting infinite mass.
	for (std::vector< Anchor >::const_iterator i = m_anchors.begin(); i != m_anchors.end(); ++i)
		clothEntity->setNodeInvMass(i->x, i->y, 0.0f);

	clothEntity->setTransform(getTransform());

	return clothEntity;
}

bool ClothEntityData::serialize(ISerializer& s)
{
	if (!world::SpatialEntityData::serialize(s))
		return false;

	s >> resource::Member< render::Shader, render::ShaderGraph >(L"shader", m_shader);
	s >> Member< uint32_t >(L"resolutionX", m_resolutionX);
	s >> Member< uint32_t >(L"resolutionY", m_resolutionY);
	s >> Member< float >(L"scale", m_scale);
	s >> MemberStlVector< Anchor, MemberComposite< Anchor > >(L"anchors", m_anchors);
	s >> Member< uint32_t >(L"solverIterations", m_solverIterations);
	s >> Member< float >(L"damping", m_damping);

	return true;
}

bool ClothEntityData::Anchor::serialize(ISerializer& s)
{
	s >> Member< uint32_t >(L"x", x);
	s >> Member< uint32_t >(L"y", y);
	return true;
}

	}
}
