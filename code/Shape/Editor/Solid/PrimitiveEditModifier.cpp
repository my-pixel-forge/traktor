#include "Core/Log/Log.h"
#include "Core/Math/Triangulator.h"
#include "Core/Math/Winding3.h"
#include "Database/Instance.h"
#include "Editor/IEditor.h"
#include "Model/Model.h"
#include "Render/PrimitiveRenderer.h"
#include "Scene/Editor/EntityAdapter.h"
#include "Scene/Editor/SceneEditorContext.h"
#include "Shape/Editor/Solid/IShape.h"
#include "Shape/Editor/Solid/PrimitiveEditModifier.h"
#include "Shape/Editor/Solid/PrimitiveEntity.h"
#include "Shape/Editor/Solid/PrimitiveEntityData.h"
#include "Shape/Editor/Solid/SolidMaterial.h"
#include "Ui/Command.h"

namespace traktor
{
    namespace shape
    {

T_IMPLEMENT_RTTI_CLASS(L"traktor.shape.PrimitiveEditModifier", PrimitiveEditModifier, scene::IModifier)

PrimitiveEditModifier::PrimitiveEditModifier(scene::SceneEditorContext* context)
:   m_context(context)
{
}

void PrimitiveEditModifier::selectionChanged()
{
    m_entityAdapters.clear();
	m_context->getEntities(m_entityAdapters, scene::SceneEditorContext::GfDefault | scene::SceneEditorContext::GfSelectedOnly | scene::SceneEditorContext::GfNoExternalChild);
}

bool PrimitiveEditModifier::cursorMoved(
    const scene::TransformChain& transformChain,
    const Vector2& cursorPosition,
    const Vector4& worldRayOrigin,
    const Vector4& worldRayDirection
)
{
    return true;
}

bool PrimitiveEditModifier::handleCommand(const ui::Command& command)
{
    if (command == L"Shape.Editor.BrowseMaterial")
    {
        Ref< db::Instance > materialInstance = m_context->getEditor()->browseInstance(type_of< SolidMaterial >());
        if (materialInstance)
        {
            for (auto entityAdapter : m_entityAdapters)
            {
                auto primitiveEntity = dynamic_type_cast< PrimitiveEntity* >(entityAdapter->getEntity());
                if (!primitiveEntity || primitiveEntity->getSelectedMaterial() == model::c_InvalidIndex)
                    continue;

                auto primitiveEntityData = mandatory_non_null_type_cast< PrimitiveEntityData* >(entityAdapter->getEntityData());
                primitiveEntityData->setMaterial(
                    primitiveEntity->getSelectedMaterial(),
                    materialInstance->getGuid()
                );
            }
        }
        return true;
    }
    else
        return false;
}

bool PrimitiveEditModifier::begin(
    const scene::TransformChain& transformChain,
    const Vector2& cursorPosition,
    const Vector4& worldRayOrigin,
    const Vector4& worldRayDirection,
    int32_t mouseButton
)
{
    for (auto entityAdapter : m_entityAdapters)
    {
        auto primitiveEntity = dynamic_type_cast< PrimitiveEntity* >(entityAdapter->getEntity());
        if (!primitiveEntity)
            continue;

        const model::Model* model = primitiveEntity->getModel();
        if (!model)
            continue;

        Transform transform = entityAdapter->getTransform();

        Scalar minK(std::numeric_limits< float >::max());
        uint32_t minMaterial = model::c_InvalidIndex;

        for (uint32_t i = 0; i < model->getPolygonCount(); ++i)
        {
            const model::Polygon& polygon = model->getPolygon(i);
            
            Winding3 w;
            for (uint32_t j = 0; j < polygon.getVertexCount(); ++j)
                w.push(transform * model->getVertexPosition(polygon.getVertex(j)));

            Scalar k;
            if (w.rayIntersection(worldRayOrigin, worldRayDirection, k))
            {
                if (k < minK)
                {
                    minMaterial = polygon.getMaterial();
                    minK = k;
                }
            }
        }

        primitiveEntity->setSelectedMaterial(minMaterial);
    }

    return false;
}

void PrimitiveEditModifier::apply(
    const scene::TransformChain& transformChain,
    const Vector2& cursorPosition,
    const Vector4& worldRayOrigin,
    const Vector4& worldRayDirection,
    const Vector4& screenDelta,
    const Vector4& viewDelta
)
{
}

void PrimitiveEditModifier::end(const scene::TransformChain& transformChain)
{
}

void PrimitiveEditModifier::draw(render::PrimitiveRenderer* primitiveRenderer) const
{
    for (auto entityAdapter : m_entityAdapters)
    {
        auto primitiveEntity = dynamic_type_cast< PrimitiveEntity* >(entityAdapter->getEntity());
        if (!primitiveEntity)
            continue;

        uint32_t selected = primitiveEntity->getSelectedMaterial();
        if (selected == model::c_InvalidIndex)
            continue;

        const model::Model* model = primitiveEntity->getModel();
        if (!model)
            continue;

        for (uint32_t i = 0; i < model->getPolygonCount(); ++i)
        {
            const model::Polygon& polygon = model->getPolygon(i);
            if (polygon.getMaterial() != selected)
                continue;

            Winding3 w;
            for (uint32_t j = 0; j < polygon.getVertexCount(); ++j)
                w.push(model->getVertexPosition(polygon.getVertex(j)));

            Plane wp;
            if (!w.getPlane(wp))
                continue;

            AlignedVector< Triangulator::Triangle > triangles;
            Triangulator().freeze(
                w.get(),
                wp.normal(),
                triangles
            );

            primitiveRenderer->pushWorld(entityAdapter->getTransform().toMatrix44());
            for (const auto& triangle : triangles)
            {
                primitiveRenderer->drawSolidTriangle(
                    w[triangle.indices[0]],
                    w[triangle.indices[1]],
                    w[triangle.indices[2]],
                    Color4ub(80, 120, 255, 120)
                );
            }
            primitiveRenderer->popWorld();
        }
    }
}

    }
}