#pragma once

#include "Core/Guid.h"
#include "Core/Object.h"
#include "Core/Ref.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_SHAPE_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
    namespace model
    {

class Model;

    }

    namespace shape
    {

class T_DLLCLASS TracerOutput : public Object
{
    T_RTTI_CLASS;

public:
    TracerOutput(
        const model::Model* model,
        const Guid& lightmapId
    );

    const model::Model* getModel() const { return m_model; }

    const Guid& getLightmapId() const { return m_lightmapId; }

private:
	Ref< const model::Model > m_model;
	Guid m_lightmapId;
};

    }
}