#pragma once

#include <string>
#include "Core/Object.h"
#include "Core/Ref.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_SCENE_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

class Guid;

	namespace editor
	{

class IPipelineCommon;
class IPipelineSettings;

	}

	namespace model
	{

class Model;

	}

	namespace world
	{

class EntityData;
class GroupComponentData;
class IEntityComponentData;

	}

	namespace scene
	{

/*! Generate model from source entity or component data, such as mesh, terrain, spline, solid etc.
 * \ingroup Scene
 */
class T_DLLCLASS IEntityReplicator : public Object
{
	T_RTTI_CLASS;

public:
	static constexpr wchar_t VisualMesh[] = L"Visual.Mesh";
	static constexpr wchar_t CollisionMesh[] = L"Collision.Mesh";
	static constexpr wchar_t CollisionShape[] = L"Collision.Shape";
	static constexpr wchar_t CollisionBody[] = L"Collision.Body";

	/*! */
	virtual bool create(const editor::IPipelineSettings* settings) = 0;

	/*! */
	virtual TypeInfoSet getSupportedTypes() const = 0;

	/*! Create visual model replica from entity or component data.
	 *
	 * \param pipelineCommon Pipeline common implementation.
	 * \param entityData Owner entity data.
	 * \param componentData Component data which we want to represent as a model.
	 * \return Model replica of entity or component.
	 */
	virtual Ref< model::Model > createVisualModel(
		editor::IPipelineCommon* pipelineCommon,
		const world::EntityData* entityData,
		const world::IEntityComponentData* componentData
	) const = 0;

	/*! Create collision model replica from entity or component data.
	 *
	 * \param pipelineCommon Pipeline common implementation.
	 * \param entityData Owner entity data.
	 * \param componentData Component data which we want to represent as a model.
	 * \return Model replica of entity or component.
	 */
	virtual Ref< model::Model > createCollisionModel(
		editor::IPipelineCommon* pipelineCommon,
		const world::EntityData* entityData,
		const world::IEntityComponentData* componentData
	) const = 0;

	/*! Transform entity and component data. */
	virtual void transform(
		world::EntityData* entityData,
		world::IEntityComponentData* componentData,
		world::GroupComponentData* outputGroup
	) const = 0;
};

	}
}