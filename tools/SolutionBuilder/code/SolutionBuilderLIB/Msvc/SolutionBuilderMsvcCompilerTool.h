/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef SolutionBuilderMsvcCompilerTool_H
#define SolutionBuilderMsvcCompilerTool_H

#include <map>
#include "SolutionBuilderLIB/Msvc/SolutionBuilderMsvcTool.h"

/*! \brief Visual Studio compiler tool. */
class SolutionBuilderMsvcCompilerTool : public SolutionBuilderMsvcTool
{
	T_RTTI_CLASS;

public:
	SolutionBuilderMsvcCompilerTool();

	virtual bool generate(
		GeneratorContext& context,
		Solution* solution,
		Project* project,
		Configuration* configuration,
		traktor::OutputStream& os
	) const;

	virtual void serialize(traktor::ISerializer& s);

private:
	bool m_resolvePaths;
	std::map< std::wstring, std::wstring > m_staticOptions;
};

#endif	// SolutionBuilderMsvcCompilerTool_H
