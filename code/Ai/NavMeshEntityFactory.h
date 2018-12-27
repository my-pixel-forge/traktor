/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_ai_NavMeshEntityFactory_H
#define traktor_ai_NavMeshEntityFactory_H

#include "World/IEntityFactory.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_AI_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace resource
	{

class IResourceManager;

	}

	namespace ai
	{

/*! \brief Navigation mesh entity factory.
 * \ingroup AI
 */
class T_DLLCLASS NavMeshEntityFactory : public world::IEntityFactory
{
	T_RTTI_CLASS;

public:
	NavMeshEntityFactory(resource::IResourceManager* resourceManager, bool editor);

	virtual const TypeInfoSet getEntityTypes() const override final;

	virtual const TypeInfoSet getEntityEventTypes() const override final;

	virtual const TypeInfoSet getEntityComponentTypes() const override final;

	virtual Ref< world::Entity > createEntity(const world::IEntityBuilder* builder, const world::EntityData& entityData) const override final;

	virtual Ref< world::IEntityEvent > createEntityEvent(const world::IEntityBuilder* builder, const world::IEntityEventData& entityEventData) const override final;

	virtual Ref< world::IEntityComponent > createEntityComponent(const world::IEntityBuilder* builder, const world::IEntityComponentData& entityComponentData) const override final;

private:
	Ref< resource::IResourceManager > m_resourceManager;
	bool m_editor;
};

	}
}

#endif	// traktor_ai_NavMeshEntityFactory_H
