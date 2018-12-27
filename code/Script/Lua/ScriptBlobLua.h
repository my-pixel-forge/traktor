/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_script_ScriptBlobLua_H
#define traktor_script_ScriptBlobLua_H

#include "Core/Misc/AutoPtr.h"
#include "Script/IScriptBlob.h"

namespace traktor
{
	namespace script
	{

class ScriptBlobLua : public IScriptBlob
{
	T_RTTI_CLASS;

public:
	virtual void serialize(ISerializer& s) override final;

private:
	friend class ScriptContextLua;
	friend class ScriptManagerLua;

	std::string m_fileName;
	std::string m_script;
};

	}
}

#endif	// traktor_script_ScriptBlobLua_H
