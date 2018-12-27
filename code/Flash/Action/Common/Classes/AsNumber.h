/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_flash_AsNumber_H
#define traktor_flash_AsNumber_H

#include "Flash/Action/ActionClass.h"

namespace traktor
{
	namespace flash
	{

class Number;

/*! \brief Number class.
 * \ingroup Flash
 */
class AsNumber : public ActionClass
{
	T_RTTI_CLASS;

public:
	AsNumber(ActionContext* context);

	virtual void initialize(ActionObject* self) override final;

	virtual void construct(ActionObject* self, const ActionValueArray& args) override final;

	virtual ActionValue xplicit(const ActionValueArray& args) override final;

private:
	std::wstring Number_toString(const Number* self) const;

	float Number_valueOf(const Number* self) const;
};

	}
}

#endif	// traktor_flash_AsNumber_H
