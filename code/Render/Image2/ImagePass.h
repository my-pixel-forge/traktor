#pragma once

#include "Core/Object.h"
#include "Core/RefArray.h"
#include "Render/Types.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_RENDER_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace render
	{

class IImageStep;

/*!
 * \ingroup Render
 */
class T_DLLCLASS ImagePass : public Object
{
	T_RTTI_CLASS;

public:

private:
	friend class ImageGraph;
	friend class ImagePassData;

	handle_t m_output;
	RefArray< const IImageStep > m_steps;
};

	}
}