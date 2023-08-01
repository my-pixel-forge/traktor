/*
 * TRAKTOR
 * Copyright (c) 2022 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Core/Io/FileSystem.h"
#include "Core/Log/Log.h"
#include "Core/Misc/String.h"
#include "Core/Serialization/DeepClone.h"
#include "Core/Settings/PropertyObject.h"
#include "Core/Settings/PropertyString.h"
#include "Editor/IPipelineCommon.h"
#include "Editor/IPipelineSettings.h"
#include "Mesh/MeshComponentData.h"
#include "Mesh/Editor/MeshAsset.h"
#include "Model/Model.h"
#include "Model/ModelCache.h"
#include "Model/ModelFormat.h"
#include "Model/Operations/MergeModel.h"
#include "Model/Operations/Transform.h"
#include "Physics/MeshShapeDesc.h"
#include "Physics/StaticBodyDesc.h"
#include "Physics/Editor/MeshAsset.h"
#include "Physics/World/RigidBodyComponentData.h"
#include "Render/Editor/Texture/TextureSet.h"
#include "Scene/Editor/Traverser.h"
#include "Shape/Editor/Prefab/PrefabComponentData.h"
#include "Shape/Editor/Prefab/PrefabEntityReplicator.h"
#include "World/EntityData.h"
#include "World/Entity/GroupComponentData.h"
#include "World/Editor/EditorAttributesComponentData.h"

namespace traktor::shape
{
	namespace
	{

const Guid c_shapeMeshAssetSeed(L"{FEC54BB1-1F55-48F5-AB87-58FE1712C42D}");

	}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.shape.PrefabEntityReplicator", 1, PrefabEntityReplicator, world::IEntityReplicator)

bool PrefabEntityReplicator::create(const editor::IPipelineSettings* settings)
{
	m_assetPath = settings->getPropertyExcludeHash< std::wstring >(L"Pipeline.AssetPath");
	m_modelCachePath = settings->getPropertyExcludeHash< std::wstring >(L"Pipeline.ModelCache.Path");
	return true;
}

TypeInfoSet PrefabEntityReplicator::getSupportedTypes() const
{
	return makeTypeInfoSet< PrefabComponentData >();
}

RefArray< const world::IEntityComponentData > PrefabEntityReplicator::getDependentComponents(
	const world::EntityData* entityData,
	const world::IEntityComponentData* componentData
) const
{
	RefArray< const world::IEntityComponentData > dependentComponentData;
	dependentComponentData.push_back(componentData);
	return dependentComponentData;
}

Ref< model::Model > PrefabEntityReplicator::createModel(
	editor::IPipelineCommon* pipelineCommon,
	const world::EntityData* entityData,
	const world::IEntityComponentData* componentData,
	Usage usage
) const
{
	if (usage == Usage::Visual)
		return createVisualModel(pipelineCommon, entityData, componentData);
	else if (usage == Usage::Collision)
		return createCollisionModel(pipelineCommon, entityData, componentData);
	else
		return nullptr;
}

void PrefabEntityReplicator::transform(
	world::EntityData* entityData,
	world::IEntityComponentData* componentData,
	world::GroupComponentData* outputGroup
) const
{
	world::GroupComponentData* groupComponentData = entityData->getComponent< world::GroupComponentData >();
	if (!groupComponentData)
	{
		log::warning << L"No group component associated with entity " << entityData->getName() << L"; required by prefab entity replicator." << Endl;
		return;
	}

	// Remove "consumed" components from prefab.
	scene::Traverser::visit(groupComponentData, [&](Ref< world::EntityData >& inoutEntityData) -> scene::Traverser::VisitorResult
	{
		if (auto editorAttributes = inoutEntityData->getComponent< world::EditorAttributesComponentData >())
		{
			if (!editorAttributes->include || editorAttributes->dynamic)
				return scene::Traverser::VrSkip;
		}

		if (auto meshComponentData = inoutEntityData->getComponent< mesh::MeshComponentData >())
			inoutEntityData->removeComponent(meshComponentData);

		if (auto rigidBodyComponentData = inoutEntityData->getComponent< physics::RigidBodyComponentData >())
			inoutEntityData->removeComponent(rigidBodyComponentData);

		return scene::Traverser::VrContinue;
	});

	// Move "non-consumed" entities from prefab into output group.
	for (auto entityData : groupComponentData->getEntityData())
		outputGroup->addEntityData(entityData);

	// Remove prefab component.
	entityData->removeComponent(componentData);
}

Ref< model::Model > PrefabEntityReplicator::createVisualModel(
	editor::IPipelineCommon* pipelineCommon,
	const world::EntityData* entityData,
	const world::IEntityComponentData* componentData
) const
{
	const world::GroupComponentData* groupComponentData = entityData->getComponent< world::GroupComponentData >();
	if (!groupComponentData)
		return nullptr;

	const Transform worldInv = entityData->getTransform().inverse();

	RefArray< model::Model > models;
	SmallMap< std::wstring, Guid > materialTemplates;
	SmallMap< std::wstring, Guid > materialTextures;

	// Collect all models from prefab component.
	scene::Traverser::visit(groupComponentData, [&](const world::EntityData* inEntityData) -> scene::Traverser::VisitorResult
	{
		// Check editor attributes component if we should include entity.
		if (auto editorAttributes = inEntityData->getComponent< world::EditorAttributesComponentData >())
		{
			if (!editorAttributes->include || editorAttributes->dynamic)
				return scene::Traverser::VrSkip;
		}		

		if (auto meshComponentData = inEntityData->getComponent< mesh::MeshComponentData >())
		{
			Ref< const mesh::MeshAsset > meshAsset = pipelineCommon->getObjectReadOnly< mesh::MeshAsset >(
				meshComponentData->getMesh()
			);
			if (!meshAsset)
			{
				log::error << L"Prefab failed; unable to read mesh asset \"" << Guid(meshComponentData->getMesh()).format() << L"\"." << Endl;
				return scene::Traverser::VrFailed;
			}

			const Path filePath = FileSystem::getInstance().getAbsolutePath(Path(m_assetPath) + meshAsset->getFileName());

			Ref< model::Model > model = model::ModelCache(m_modelCachePath).get(filePath, meshAsset->getImportFilter());
			if (!model)
			{
				log::error << L"Prefab failed; unable to read model \"" << filePath.getPathName() << L"\"." << Endl;
				return scene::Traverser::VrFailed;
			}

			model::Transform(
				translate(meshAsset->getOffset()) *
				scale(meshAsset->getScaleFactor(), meshAsset->getScaleFactor(), meshAsset->getScaleFactor())
			).apply(*model);
			model::Transform(
				(worldInv * inEntityData->getTransform()).toMatrix44()
			).apply(*model);

			model->clear(model::Model::CfColors | model::Model::CfJoints);
			models.push_back(model);

			// Insert material templates.
			for (const auto& mt : meshAsset->getMaterialTemplates())
			 	materialTemplates[mt.first] = mt.second;

			// First use textures from texture set.
			const auto& textureSetId = meshAsset->getTextureSet();
			if (textureSetId.isNotNull())
			{
			 	Ref< const render::TextureSet > textureSet = pipelineCommon->getObjectReadOnly< render::TextureSet >(textureSetId);
			 	if (!textureSet)
			 	{
			 		log::error << L"Prefab failed; unable to read texture set \"" << textureSetId.format() << L"\"." << Endl;
			 		return scene::Traverser::VrFailed;
			 	}
			 	for (auto mt : textureSet->get())
			 		materialTextures[mt.first] = mt.second;
			}

			// Then let explicit material textures override those from a texture set.
			for (auto mt : meshAsset->getMaterialTextures())
			 	materialTextures[mt.first] = mt.second;
		}

		return scene::Traverser::VrContinue;
	});


	log::info << L"Prefab replicator collected " << models.size() << L" models to be merged." << Endl;

	// Create merged model.
	Ref< model::Model > outputModel = new model::Model();
	for (auto mdl : models)
		model::MergeModel(*mdl, Transform::identity(), 0.001f).apply(*outputModel);

	// Create a mesh asset; used by bake pipeline to set appropriate materials.
	Ref< mesh::MeshAsset > outputMeshAsset = new mesh::MeshAsset();
	outputMeshAsset->setMeshType(mesh::MeshAsset::MtStatic);
	outputMeshAsset->setMaterialTemplates(materialTemplates);
	outputMeshAsset->setMaterialTextures(materialTextures);
	outputModel->setProperty< PropertyObject >(type_name(outputMeshAsset), outputMeshAsset);

	return outputModel;
}

Ref< model::Model > PrefabEntityReplicator::createCollisionModel(
	editor::IPipelineCommon* pipelineCommon,
	const world::EntityData* entityData,
	const world::IEntityComponentData* componentData
) const
{
	const PrefabComponentData* prefabComponentData = mandatory_non_null_type_cast< const PrefabComponentData* >(componentData);
	Transform worldInv = entityData->getTransform().inverse();

	RefArray< model::Model > models;
	SmallMap< std::wstring, Guid > materialPhysics;
	SmallSet< resource::Id< physics::CollisionSpecification > > collisionGroup;
	SmallSet< resource::Id< physics::CollisionSpecification > > collisionMask;
	float friction = 0.0f;
	float restitution = 0.0f;

	// Collect all models from prefab component.
	scene::Traverser::visit(prefabComponentData, [&](const world::EntityData* inEntityData) -> scene::Traverser::VisitorResult
	{
		// Check editor attributes component if we should include entity.
		if (auto editorAttributes = inEntityData->getComponent< world::EditorAttributesComponentData >())
		{
			if (!editorAttributes->include || editorAttributes->dynamic)
				return scene::Traverser::VrSkip;
		}		

		if (auto rigidBodyComponentData = inEntityData->getComponent< physics::RigidBodyComponentData >())
		{
			auto bodyDesc = dynamic_type_cast< const physics::StaticBodyDesc* >(rigidBodyComponentData->getBodyDesc());
			if (!bodyDesc)
				return scene::Traverser::VrContinue;

			auto meshShape = dynamic_type_cast< const physics::MeshShapeDesc* >(rigidBodyComponentData->getBodyDesc()->getShape());
			if (!meshShape)
				return scene::Traverser::VrContinue;

			Ref< const physics::MeshAsset > meshAsset = pipelineCommon->getObjectReadOnly< physics::MeshAsset >(meshShape->getMesh());
			if (!meshAsset)
			{
				log::error << L"Prefab failed; unable to read collision mesh \"" << Guid(meshShape->getMesh()).format() << L"\"." << Endl;
				return scene::Traverser::VrFailed;
			}

			Path filePath = FileSystem::getInstance().getAbsolutePath(Path(m_assetPath) + meshAsset->getFileName());
			Ref< model::Model > shapeModel = model::ModelCache(m_modelCachePath).get(filePath, meshAsset->getImportFilter());
			if (!shapeModel)
			{
				log::error << L"Prefab failed; unable to read collision model \"" << filePath.getPathName() << L"\"." << Endl;
				return scene::Traverser::VrFailed;
			}

			model::Transform(
				(worldInv * inEntityData->getTransform()).toMatrix44()
			).apply(*shapeModel);

			models.push_back(shapeModel);

			collisionGroup.insert(meshShape->getCollisionGroup().begin(), meshShape->getCollisionGroup().end());
			collisionMask.insert(meshShape->getCollisionMask().begin(), meshShape->getCollisionMask().end());

			for (auto it : meshAsset->getMaterials())
				materialPhysics[it.first] = it.second;

			friction += bodyDesc->getFriction();
			restitution += bodyDesc->getRestitution();
		}

		return scene::Traverser::VrContinue;
	});

	if (models.empty())
		return nullptr;

	// Use average material properties as default for physics shape.
	friction /= (float)models.size();
	restitution /= (float)models.size();

	// Create merged model.
	Ref< model::Model > outputModel = new model::Model();
	for (auto mdl : models)
		model::MergeModel(*mdl, Transform::identity(), 0.001f).apply(*outputModel);

	// Create shape descriptor; used by bake pipeline to set appropriate collision materials.
 	Ref< physics::MeshAsset > outputShapeMeshAsset = new physics::MeshAsset();
 	outputShapeMeshAsset->setCalculateConvexHull(false);
 	outputShapeMeshAsset->setMaterials(materialPhysics);
	outputModel->setProperty< PropertyObject >(type_name(outputShapeMeshAsset), outputShapeMeshAsset);

	Ref< physics::ShapeDesc > outputShapeDesc = new physics::ShapeDesc();
	outputShapeDesc->setCollisionGroup(collisionGroup);
	outputShapeDesc->setCollisionMask(collisionMask);
	outputModel->setProperty< PropertyObject >(type_name(outputShapeDesc), outputShapeDesc);

	Ref< physics::StaticBodyDesc > outputBodyDesc = new physics::StaticBodyDesc();
	outputBodyDesc->setFriction(friction);
	outputBodyDesc->setRestitution(restitution);
	outputModel->setProperty< PropertyObject >(type_name(outputBodyDesc), outputBodyDesc);

	return outputModel;
}

}