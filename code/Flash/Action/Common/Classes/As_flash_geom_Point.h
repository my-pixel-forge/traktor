/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_flash_As_flash_geom_Point_H
#define traktor_flash_As_flash_geom_Point_H

#include "Flash/Action/ActionClass.h"

namespace traktor
{
	namespace flash
	{

struct CallArgs;

/*! \brief Point class.
 * \ingroup Flash
 */
class As_flash_geom_Point : public ActionClass
{
	T_RTTI_CLASS;

public:
	As_flash_geom_Point(ActionContext* context);

	virtual void initialize(ActionObject* self) override final;

	virtual void construct(ActionObject* self, const ActionValueArray& args) override final;

	virtual ActionValue xplicit(const ActionValueArray& args) override final;

private:
	void Point_distance(CallArgs& ca);

	void Point_interpolate(CallArgs& ca);

	void Point_polar(CallArgs& ca);
};

	}
}

#endif	// traktor_flash_As_flash_geom_Point_H
