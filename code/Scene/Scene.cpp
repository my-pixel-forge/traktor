#include "Scene/Scene.h"
#include "World/Entity/Entity.h"
#include "World/WorldRenderSettings.h"
#include "World/PostProcess/PostProcess.h"

namespace traktor
{
	namespace scene
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.scene.Scene", Scene, Object)

Scene::Scene(
	ISceneController* controller,
	world::IEntityManager* entityManager,
	world::Entity* rootEntity,
	world::WorldRenderSettings* worldRenderSettings,
	world::PostProcess* postProcess
)
:	m_controller(controller)
,	m_entityManager(entityManager)
,	m_rootEntity(rootEntity)
,	m_worldRenderSettings(worldRenderSettings)
,	m_postProcess(postProcess)
{
}

Scene::~Scene()
{
	destroy();
}

void Scene::destroy()
{
	if (m_postProcess)
	{
		m_postProcess->destroy();
		m_postProcess = 0;
	}

	if (m_rootEntity)
	{
		m_rootEntity->destroy();
		m_rootEntity = 0;
	}

	m_entityManager = 0;
	m_controller = 0;
	m_worldRenderSettings = 0;
}

ISceneController* Scene::getController() const
{
	return m_controller;
}

world::IEntityManager* Scene::getEntityManager() const
{
	return m_entityManager;
}

world::Entity* Scene::getRootEntity() const
{
	return m_rootEntity;
}

world::WorldRenderSettings* Scene::getWorldRenderSettings() const
{
	return m_worldRenderSettings;
}

world::PostProcess* Scene::getPostProcess() const
{
	return m_postProcess;
}

	}
}
