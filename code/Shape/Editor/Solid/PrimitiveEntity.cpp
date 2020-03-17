#include "Model/Model.h"
#include "Shape/Editor/Solid/PrimitiveEntity.h"

namespace traktor
{
	namespace shape
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.shape.PrimitiveEntity", PrimitiveEntity, world::Entity)

PrimitiveEntity::PrimitiveEntity(const PrimitiveEntityData* data, const Transform& transform)
:	world::Entity(transform)
,	m_data(data)
,   m_selectedMaterial(model::c_InvalidIndex)
,   m_dirty(true)
{
}

void PrimitiveEntity::setTransform(const Transform& transform)
{
	if (getTransform() != transform)
	{
		setTransform(transform);
		m_dirty = true;
	}
}

Aabb3 PrimitiveEntity::getBoundingBox() const
{
	return m_boundingBox;
}

void PrimitiveEntity::update(const world::UpdateParams& update)
{
}

	}
}