#pragma once

#include "Core/Guid.h"
#include "Shape/Editor/Solid/IShape.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_SHAPE_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace shape
	{
	
class T_DLLCLASS Box : public IShape
{
	T_RTTI_CLASS;

public:
	Box();

	virtual Ref< model::Model > createModel() const override final;

	virtual void createAnchors(AlignedVector< Vector4 >& outAnchors) const override final;

	virtual void serialize(ISerializer& s) override final;

private:
	Vector4 m_extent;
	Guid m_materials[6];
};

	}
}