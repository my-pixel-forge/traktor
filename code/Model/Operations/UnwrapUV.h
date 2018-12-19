#pragma once

#include "Model/IModelOperation.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_MODEL_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace model
	{

/*! \brief
 * \ingroup Model
 */
class T_DLLCLASS UnwrapUV : public IModelOperation
{
	T_RTTI_CLASS;

public:
	UnwrapUV(int32_t channel);

	virtual bool apply(Model& model) const T_OVERRIDE T_FINAL;

private:
	int32_t m_channel;
};

	}
}
