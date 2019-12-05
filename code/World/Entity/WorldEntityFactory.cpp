#include "Render/ICubeTexture.h"
#include "Render/IRenderSystem.h"
#include "Render/Shader.h"
#include "Resource/IResourceManager.h"
#include "World/IEntityBuilder.h"
#include "World/Entity/CameraComponent.h"
#include "World/Entity/CameraComponentData.h"
#include "World/Entity/ComponentEntity.h"
#include "World/Entity/ComponentEntityData.h"
#include "World/Entity/DecalComponent.h"
#include "World/Entity/DecalComponentData.h"
#include "World/Entity/DecalEvent.h"
#include "World/Entity/DecalEventData.h"
#include "World/Entity/ExternalEntityData.h"
#include "World/Entity/GroupComponent.h"
#include "World/Entity/GroupComponentData.h"
#include "World/Entity/GroupEntity.h"
#include "World/Entity/GroupEntityData.h"
#include "World/Entity/LightComponent.h"
#include "World/Entity/LightComponentData.h"
#include "World/Entity/ProbeComponent.h"
#include "World/Entity/ProbeComponentData.h"
#include "World/Entity/ScriptComponent.h"
#include "World/Entity/ScriptComponentData.h"
#include "World/Entity/VolumeComponent.h"
#include "World/Entity/VolumeComponentData.h"
#include "World/Entity/WorldEntityFactory.h"

namespace traktor
{
	namespace world
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.world.WorldEntityFactory", WorldEntityFactory, IEntityFactory)

WorldEntityFactory::WorldEntityFactory(resource::IResourceManager* resourceManager, render::IRenderSystem* renderSystem, bool editor)
:	m_resourceManager(resourceManager)
,	m_renderSystem(renderSystem)
,	m_editor(editor)
{
}

const TypeInfoSet WorldEntityFactory::getEntityTypes() const
{
	TypeInfoSet typeSet;
	typeSet.insert< ComponentEntityData >();
	typeSet.insert< ExternalEntityData >();
	typeSet.insert< GroupEntityData >();
	return typeSet;
}

const TypeInfoSet WorldEntityFactory::getEntityEventTypes() const
{
	TypeInfoSet typeSet;
	typeSet.insert< DecalEventData >();
	return typeSet;
}

const TypeInfoSet WorldEntityFactory::getEntityComponentTypes() const
{
	TypeInfoSet typeSet;
	typeSet.insert< CameraComponentData >();
	typeSet.insert< DecalComponentData >();
	typeSet.insert< GroupComponentData >();
	typeSet.insert< LightComponentData >();
	typeSet.insert< ProbeComponentData >();
	typeSet.insert< ScriptComponentData >();
	typeSet.insert< VolumeComponentData >();
	return typeSet;
}

Ref< Entity > WorldEntityFactory::createEntity(const IEntityBuilder* builder, const EntityData& entityData) const
{
	if (const ExternalEntityData* externalEntityData = dynamic_type_cast< const ExternalEntityData* >(&entityData))
	{
		resource::Proxy< EntityData > resolvedEntityData;
		if (!m_resourceManager->bind(externalEntityData->getEntityData(), resolvedEntityData))
			return nullptr;

		resolvedEntityData->setName(externalEntityData->getName());
		resolvedEntityData->setTransform(externalEntityData->getTransform());

		if (m_editor)
		{
			// Due to scene editor collecting adapters we cannot continue to create
			// using the aggregated builder thus we have to manually create child entities.
			const IEntityFactory* factory = builder->getFactory(resolvedEntityData);
			if (factory)
				return factory->createEntity(builder->getCompositeEntityBuilder(), *resolvedEntityData.getResource());
			else
				return nullptr;
		}
		else
			return builder->create(resolvedEntityData.getResource());
	}

	if (const GroupEntityData* groupData = dynamic_type_cast< const GroupEntityData* >(&entityData))
	{
		Ref< GroupEntity > groupEntity = new GroupEntity(
			groupData->getTransform(),
			groupData->getMask()
		);
		for (auto entityData : groupData->getEntityData())
		{
			Ref< Entity > childEntity = builder->create(entityData);
			if (childEntity)
				groupEntity->addEntity(childEntity);
		}
		return groupEntity;
	}

	if (const ComponentEntityData* componentData = dynamic_type_cast< const ComponentEntityData* >(&entityData))
	{
		Ref< ComponentEntity > componentEntity = new ComponentEntity(componentData->getTransform());
		for (uint32_t i = 0; i < componentData->m_components.size(); ++i)
		{
			Ref< IEntityComponent > component = builder->create(componentData->m_components[i]);
			if (!component)
			{
				if (!m_editor)
					return nullptr;
				else
					continue;
			}
			componentEntity->setComponent(component);
		}
		return componentEntity;
	}

	return nullptr;
}

Ref< IEntityEvent > WorldEntityFactory::createEntityEvent(const IEntityBuilder* builder, const IEntityEventData& entityEventData) const
{
	if (const DecalEventData* decalData = dynamic_type_cast< const DecalEventData* >(&entityEventData))
	{
		Ref< DecalEvent > decal = new DecalEvent();

		decal->m_size = decalData->getSize();
		decal->m_thickness = decalData->getThickness();
		decal->m_alpha = decalData->getAlpha();
		decal->m_cullDistance = decalData->getCullDistance();

		if (m_resourceManager->bind(decalData->getShader(), decal->m_shader))
			return decal;
	}
	return nullptr;
}

Ref< IEntityComponent > WorldEntityFactory::createEntityComponent(const world::IEntityBuilder* builder, const IEntityComponentData& entityComponentData) const
{
	if (const CameraComponentData* cameraComponentData = dynamic_type_cast< const CameraComponentData* >(&entityComponentData))
		return new CameraComponent(cameraComponentData);

	if (const DecalComponentData* decalComponentData = dynamic_type_cast< const DecalComponentData* >(&entityComponentData))
	{
		resource::Proxy< render::Shader > shader;
		if (!m_resourceManager->bind(decalComponentData->getShader(), shader))
			return nullptr;

		Ref< DecalComponent > decalComponent = new DecalComponent(
			decalComponentData->getSize(),
			decalComponentData->getThickness(),
			decalComponentData->getAlpha(),
			decalComponentData->getCullDistance(),
			shader
		);

		return decalComponent;
	}

	if (const GroupComponentData* groupComponentData = dynamic_type_cast< const GroupComponentData* >(&entityComponentData))
	{
		Ref< GroupComponent > groupComponent = new GroupComponent();
		for (auto entityData : groupComponentData->getEntityData())
		{
			Ref< Entity > childEntity = builder->create(entityData);
			if (childEntity)
				groupComponent->addEntity(childEntity);
		}
		return groupComponent;
	}

	if (const LightComponentData* lightComponentData = dynamic_type_cast<const LightComponentData*>(&entityComponentData))
	{
		return new LightComponent(
			lightComponentData->getLightType(),
			lightComponentData->getColor() * Scalar(lightComponentData->getIntensity()),
			lightComponentData->getCastShadow(),
			lightComponentData->getRange(),
			lightComponentData->getRadius(),
			lightComponentData->getFlickerAmount(),
			lightComponentData->getFlickerFilter()
		);
	}

	if (const ProbeComponentData* probeComponentData = dynamic_type_cast< const ProbeComponentData* >(&entityComponentData))
	{
		resource::Proxy< render::ICubeTexture > texture;
		bool dirty = false;

		if (probeComponentData->getTexture())
		{
			if (!m_resourceManager->bind(probeComponentData->getTexture(), texture))
				return nullptr;
		}
		else
		{
			render::CubeTextureCreateDesc ctcd;
#if !defined(__ANDROID__)
			ctcd.side = 1024;
#else
			ctcd.side = 128;
#endif
			ctcd.mipCount = log2(ctcd.side ) + 1;
			ctcd.format = render::TfR11G11B10F;
			ctcd.sRGB = false;
			ctcd.immutable = false;

			texture = resource::Proxy< render::ICubeTexture >(m_renderSystem->createCubeTexture(ctcd, T_FILE_LINE_W));
			if (!texture)
				return nullptr;

			dirty = true;
		}

		return new ProbeComponent(
			texture,
			probeComponentData->getIntensity(),
			probeComponentData->getLocal(),
			probeComponentData->getVolume(),
			dirty
		);
	}

	if (const ScriptComponentData* scriptComponentData = dynamic_type_cast< const ScriptComponentData* >(&entityComponentData))
	{
		if (!m_editor || scriptComponentData->getEditorSupport())
			return scriptComponentData->createComponent(m_resourceManager);
		else
			return nullptr;
	}

	if (const VolumeComponentData* volumeComponentData = dynamic_type_cast< const VolumeComponentData* >(&entityComponentData))
		return new VolumeComponent(volumeComponentData);

	return nullptr;
}

	}
}
