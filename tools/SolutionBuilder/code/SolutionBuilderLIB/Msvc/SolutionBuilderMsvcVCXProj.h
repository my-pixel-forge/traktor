#ifndef SolutionBuilderMsvcVCXProj_H
#define SolutionBuilderMsvcVCXProj_H

#include "SolutionBuilderLIB/Msvc/SolutionBuilderMsvcProject.h"

class Configuration;
class ProjectItem;
class SolutionBuilderMsvcVCXDefinition;
class SolutionBuilderMsvcVCXBuildTool;

/*! \brief Visual Studio solution project settings. */
class SolutionBuilderMsvcVCXProj : public SolutionBuilderMsvcProject
{
	T_RTTI_CLASS(SolutionBuilderMsvcVCXProj)

public:
	virtual std::wstring getPlatform() const;

	virtual bool getInformation(
		GeneratorContext& context,
		Solution* solution,
		Project* project,
		std::wstring& outProjectPath,
		std::wstring& outProjectFileName,
		std::wstring& outProjectGuid
	) const;

	virtual bool generate(
		GeneratorContext& context,
		Solution* solution,
		Project* project
	) const;

	virtual bool serialize(traktor::Serializer& s);

private:
	std::wstring m_platform;
	std::wstring m_keyword;
	traktor::RefArray< SolutionBuilderMsvcVCXDefinition > m_buildDefinitionsDebug[4];
	traktor::RefArray< SolutionBuilderMsvcVCXDefinition > m_buildDefinitionsRelease[4];
	traktor::RefArray< SolutionBuilderMsvcVCXBuildTool > m_buildTools;

	bool collectFiles(
		Project* project,
		ProjectItem* item,
		std::vector< traktor::Path >& outFiles
	) const;
};

#endif	// SolutionBuilderMsvcVCXProj_H
