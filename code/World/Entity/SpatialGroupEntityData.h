#ifndef traktor_world_SpatialGroupEntityData_H
#define traktor_world_SpatialGroupEntityData_H

#include "Core/Heap/Ref.h"
#include "World/Entity/SpatialEntityData.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_WORLD_EXPORT)
#define T_DLLCLASS T_DLLEXPORT
#else
#define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace world
	{

class EntityInstance;

/*! \brief Spatial group entity data.
 * \ingroup World
 */
class T_DLLCLASS SpatialGroupEntityData : public SpatialEntityData
{
	T_RTTI_CLASS(SpatialGroupEntityData)

public:
	void addInstance(EntityInstance* instance);

	void removeInstance(EntityInstance* instance);

	void removeAllInstances();

	RefArray< EntityInstance >& getInstances();

	const RefArray< EntityInstance >& getInstances() const;

	virtual void setTransform(const Matrix44& transform);
	
	virtual bool serialize(Serializer& s);
	
private:
	RefArray< EntityInstance > m_instances;
};
	
	}
}

#endif	// traktor_world_SpatialGroupEntityData_H
