#ifndef FlattenDefinitionsTool_H
#define FlattenDefinitionsTool_H

#include "CustomTool.h"

class FlattenDefinitionsTool : public CustomTool
{
	T_RTTI_CLASS;

public:
	bool execute(traktor::ui::Widget* parent, Solution* solution);
};

#endif	// FlattenDefinitionsTool_H
