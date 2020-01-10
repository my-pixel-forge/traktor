#pragma once

#include "Core/RefArray.h"
#include "Core/Math/IntervalTransform.h"
#include "World/Entity.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_WORLD_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace world
	{

class IEntityComponent;
class IWorldRenderPass;
class WorldContext;
class WorldRenderView;

/*! \brief
 * \ingroup World
 */
class T_DLLCLASS ComponentEntity : public Entity
{
	T_RTTI_CLASS;

public:
	ComponentEntity();

	explicit ComponentEntity(const Transform& transform);

	virtual ~ComponentEntity();

	virtual void destroy() override;

	virtual void setTransform(const Transform& transform) override;

	virtual Transform getTransform() const override;

	virtual Aabb3 getBoundingBox() const override;

	virtual void update(const UpdateParams& update) override;

	/*! Set component in character instance.
	 *
	 * \param component Component instance.
	 */
	void setComponent(IEntityComponent* component);

	/*! Get component of type. */
	IEntityComponent* getComponent(const TypeInfo& componentType) const;

	/*! Get component of type. */
	template < typename ComponentType >
	ComponentType* getComponent() const
	{
		return checked_type_cast< ComponentType* >(getComponent(type_of< ComponentType >()));
	}

	/*! Get all components. */
	const RefArray< IEntityComponent >& getComponents() const
	{
		return m_components;
	}

private:
	friend class WorldEntityFactory;

	Transform m_transform;
	RefArray< IEntityComponent > m_components;
};

	}
}
