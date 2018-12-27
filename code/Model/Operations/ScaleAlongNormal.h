/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_model_ScaleAlongNormal_H
#define traktor_model_ScaleAlongNormal_H

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
class T_DLLCLASS ScaleAlongNormal : public IModelOperation
{
	T_RTTI_CLASS;

public:
	ScaleAlongNormal(float distance);

	virtual bool apply(Model& model) const override final;

private:
	float m_distance;
};

	}
}

#endif	// traktor_model_ScaleAlongNormal_H
