#include <Core/Io/FileSystem.h>
#include <Core/Io/DynamicMemoryStream.h>
#include <Core/Io/FileOutputStream.h>
#include <Core/Io/AnsiEncoding.h>
#include <Core/Serialization/ISerializer.h>
#include <Core/Serialization/Member.h>
#include <Core/Serialization/MemberStaticArray.h>
#include <Core/Serialization/MemberStl.h>
#include <Core/Serialization/MemberRefArray.h>
#include <Core/Misc/String.h>
#include <Core/Misc/MD5.h>
#include <Core/Log/Log.h>
#include "SolutionBuilderLIB/Solution.h"
#include "SolutionBuilderLIB/Project.h"
#include "SolutionBuilderLIB/ProjectDependency.h"
#include "SolutionBuilderLIB/ExternalDependency.h"
#include "SolutionBuilderLIB/Configuration.h"
#include "SolutionBuilderLIB/Filter.h"
#include "SolutionBuilderLIB/File.h"
#include "SolutionBuilderLIB/Msvc/GeneratorContext.h"
#include "SolutionBuilderLIB/Msvc/SolutionBuilderMsvcVCXDefinition.h"
#include "SolutionBuilderLIB/Msvc/SolutionBuilderMsvcVCXBuildTool.h"
#include "SolutionBuilderLIB/Msvc/SolutionBuilderMsvcVCXProj.h"
#include "SolutionBuilderLIB/Msvc/SolutionBuilderMsvcVCXPropertyGroup.h"

using namespace traktor;

namespace
{

	std::wstring systemPath(const Path& path)
	{
		return replaceAll< std::wstring >(path.getPathName(), L'/', L'\\');
	}

}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"SolutionBuilderMsvcVCXProj", 4, SolutionBuilderMsvcVCXProj, SolutionBuilderMsvcProject)

SolutionBuilderMsvcVCXProj::SolutionBuilderMsvcVCXProj()
{
	m_targetPrefixes[0] = L"";
	m_targetPrefixes[1] = L"";
	m_targetPrefixes[2] = L"";
	m_targetPrefixes[3] = L"";
	m_targetExts[0] = L".lib";
	m_targetExts[1] = L".dll";
	m_targetExts[2] = L".exe";
	m_targetExts[3] = L".exe";
}

std::wstring SolutionBuilderMsvcVCXProj::getPlatform() const
{
	return m_platform;
}

bool SolutionBuilderMsvcVCXProj::getInformation(
	GeneratorContext& context,
	Solution* solution,
	Project* project,
	std::wstring& outProjectPath,
	std::wstring& outProjectFileName,
	std::wstring& outProjectGuid
) const
{
	outProjectPath = solution->getRootPath() + L"/" + project->getName();
	outProjectFileName = outProjectPath + L"/" + project->getName() + L".vcxproj";
	outProjectGuid = context.generateGUID(outProjectFileName);
	return true;
}

bool SolutionBuilderMsvcVCXProj::generate(
	GeneratorContext& context,
	Solution* solution,
	Project* project
) const
{
	if (!generateProject(context, solution, project))
		return false;
	
	if (!generateFilters(context, solution, project))
		return false;

	return true;
}

void SolutionBuilderMsvcVCXProj::serialize(traktor::ISerializer& s)
{
	std::wstring toolset;

	s >> Member< std::wstring >(L"platform", m_platform);
	s >> Member< std::wstring >(L"keyword", m_keyword);
	
	if (s.getVersion() >= 1 && s.getVersion() < 3)
		s >> Member< std::wstring >(L"toolset", toolset);

	if (s.getVersion() >= 2)
	{
		s >> MemberStaticArray< std::wstring, sizeof_array(m_targetPrefixes) >(L"targetPrefixes", m_targetPrefixes);
		s >> MemberStaticArray< std::wstring, sizeof_array(m_targetExts) >(L"targetExts", m_targetExts);
	}

	if (s.getVersion() >= 4)
		s >> MemberRefArray< SolutionBuilderMsvcVCXPropertyGroup >(L"propertyGroups", m_propertyGroups);

	if (s.getVersion() >= 3)
	{
		const wchar_t* itemNames[] = { L"staticLibrary", L"sharedLibrary", L"executable", L"executableConsole" };

		s >> MemberStaticArray<
			std::map< std::wstring, std::wstring >,
			4,
			MemberStlMap< std::wstring, std::wstring >
		>(L"configurationDefinitionsDebug", m_configurationDefinitionsDebug, itemNames);

		s >> MemberStaticArray<
			std::map< std::wstring, std::wstring >,
			4,
			MemberStlMap< std::wstring, std::wstring >
		>(L"configurationDefinitionsRelease", m_configurationDefinitionsRelease, itemNames);
	}
	else
	{
		m_configurationDefinitionsDebug[0].insert(std::make_pair(L"CharacterSet", L"MultiByte"));
		m_configurationDefinitionsDebug[0].insert(std::make_pair(L"ConfigurationType", L"StaticLibrary"));
		m_configurationDefinitionsDebug[0].insert(std::make_pair(L"WholeProgramOptimization", L"false"));
		m_configurationDefinitionsDebug[0].insert(std::make_pair(L"PlatformToolset", toolset));

		m_configurationDefinitionsDebug[1].insert(std::make_pair(L"CharacterSet", L"MultiByte"));
		m_configurationDefinitionsDebug[1].insert(std::make_pair(L"ConfigurationType", L"DynamicLibrary"));
		m_configurationDefinitionsDebug[1].insert(std::make_pair(L"WholeProgramOptimization", L"false"));
		m_configurationDefinitionsDebug[1].insert(std::make_pair(L"PlatformToolset", toolset));

		m_configurationDefinitionsDebug[2].insert(std::make_pair(L"CharacterSet", L"MultiByte"));
		m_configurationDefinitionsDebug[2].insert(std::make_pair(L"ConfigurationType", L"Application"));
		m_configurationDefinitionsDebug[2].insert(std::make_pair(L"WholeProgramOptimization", L"false"));
		m_configurationDefinitionsDebug[2].insert(std::make_pair(L"PlatformToolset", toolset));

		m_configurationDefinitionsDebug[3].insert(std::make_pair(L"CharacterSet", L"MultiByte"));
		m_configurationDefinitionsDebug[3].insert(std::make_pair(L"ConfigurationType", L"Application"));
		m_configurationDefinitionsDebug[3].insert(std::make_pair(L"WholeProgramOptimization", L"false"));
		m_configurationDefinitionsDebug[3].insert(std::make_pair(L"PlatformToolset", toolset));

		m_configurationDefinitionsRelease[0] = m_configurationDefinitionsDebug[0];
		m_configurationDefinitionsRelease[1] = m_configurationDefinitionsDebug[1];
		m_configurationDefinitionsRelease[2] = m_configurationDefinitionsDebug[2];
		m_configurationDefinitionsRelease[3] = m_configurationDefinitionsDebug[3];
	}

	s >> MemberStaticArray<
			RefArray< SolutionBuilderMsvcVCXDefinition >,
			sizeof_array(m_buildDefinitionsDebug),
			MemberRefArray< SolutionBuilderMsvcVCXDefinition >
		>(L"buildDefinitionsDebug", m_buildDefinitionsDebug);
	s >> MemberStaticArray<
			RefArray< SolutionBuilderMsvcVCXDefinition >,
			sizeof_array(m_buildDefinitionsRelease),
			MemberRefArray< SolutionBuilderMsvcVCXDefinition >
		>(L"buildDefinitionsRelease", m_buildDefinitionsRelease);
	s >> MemberRefArray< SolutionBuilderMsvcVCXBuildTool >(L"buildTools", m_buildTools);
}

bool SolutionBuilderMsvcVCXProj::generateProject(
	GeneratorContext& context,
	Solution* solution,
	Project* project
) const
{
	std::wstring projectPath, projectFileName, projectGuid;
	if (!getInformation(
		context,
		solution,
		project,
		projectPath,
		projectFileName,
		projectGuid
	))
		return false;

	if (!FileSystem::getInstance().makeDirectory(projectPath))
		return false;

	traktor::log::info << L"Generating msbuild project \"" << projectFileName << L"\"" << Endl;

	context.set(L"PROJECT_PLATFORM", m_platform);
	context.set(L"PROJECT_NAME", project->getName());
	context.set(L"PROJECT_PATH", projectPath);
	context.set(L"PROJECT_FILENAME", projectFileName);
	context.set(L"PROJECT_GUID", projectGuid);

	context.set(L"MODULE_DEFINITION_FILE", L"");
	findDefinitions(context, solution, project, project->getItems());

	std::vector< uint8_t > buffer;
	buffer.reserve(40000);

	DynamicMemoryStream bufferStream(buffer, false, true);
	FileOutputStream os(&bufferStream, new AnsiEncoding());

	os << L"<Project DefaultTargets=\"Build\" ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">" << Endl;
	os << IncreaseIndent;

	// Custom property groups.
	for (RefArray< SolutionBuilderMsvcVCXPropertyGroup >::const_iterator i = m_propertyGroups.begin(); i != m_propertyGroups.end(); ++i)
		(*i)->generate(
			context,
			solution,
			project,
			os
		);

	// Configurations
	os << L"<ItemGroup Label=\"ProjectConfigurations\">" << Endl;
	os << IncreaseIndent;

	const RefArray< Configuration >& configurations = project->getConfigurations();
	for (RefArray< Configuration >::const_iterator i = configurations.begin(); i != configurations.end(); ++i)
	{
		const Configuration* configuration = *i;
		T_ASSERT (configuration);

		os << L"<ProjectConfiguration Include=\"" << configuration->getName() << L"|" << m_platform << L"\">" << Endl;
		os << IncreaseIndent;

		os << L"<Configuration>" << configuration->getName() << L"</Configuration>" << Endl;
		os << L"<Platform>" << m_platform << L"</Platform>" << Endl;

		os << DecreaseIndent;
		os << L"</ProjectConfiguration>" << Endl;
	}

	os << DecreaseIndent;
	os << L"</ItemGroup>" << Endl;

	// Globals
	os << L"<PropertyGroup Label=\"Globals\">" << Endl;
	os << IncreaseIndent;

	if (!m_keyword.empty())
		os << L"<Keyword>" << m_keyword << L"</Keyword>" << Endl;

	os << L"<ProjectGUID>" << projectGuid << L"</ProjectGUID>" << Endl;
	os << L"<RootNamespace>" << project->getName() << L"</RootNamespace>" << Endl;

	os << DecreaseIndent;
	os << L"</PropertyGroup>" << Endl;

	os << L"<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />" << Endl;

	// Configurations
	for (RefArray< Configuration >::const_iterator i = configurations.begin(); i != configurations.end(); ++i)
	{
		const Configuration* configuration = *i;
		T_ASSERT (configuration);

		os << L"<PropertyGroup Label=\"Configuration\" Condition=\"'$(Configuration)|$(Platform)'=='" << configuration->getName() << L"|" << m_platform << L"'\">" << Endl;
		os << IncreaseIndent;

		if (configuration->getTargetProfile() == Configuration::TpDebug)
		{
			const std::map< std::wstring, std::wstring >& cd = m_configurationDefinitionsDebug[configuration->getTargetFormat()];
			for (std::map< std::wstring, std::wstring >::const_iterator i = cd.begin(); i != cd.end(); ++i)
				os << L"<" << i->first << L">" << i->second << L"</" << i->first << L">" << Endl;
		}
		else
		{
			const std::map< std::wstring, std::wstring >& cd = m_configurationDefinitionsRelease[configuration->getTargetFormat()];
			for (std::map< std::wstring, std::wstring >::const_iterator i = cd.begin(); i != cd.end(); ++i)
				os << L"<" << i->first << L">" << i->second << L"</" << i->first << L">" << Endl;
		}

		os << DecreaseIndent;
		os << L"</PropertyGroup>" << Endl;
	}

	os << L"<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />" << Endl;
	os << L"<ImportGroup Label=\"PropertySheets\">" << Endl;
	os << IncreaseIndent;
	os << L"<Import Project=\"$(LocalAppData)\\Microsoft\\VisualStudio\\10.0\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(LocalAppData)\\Microsoft\\VisualStudio\\10.0\\Microsoft.Cpp.$(Platform).user.props')\" />" << Endl;
	os << DecreaseIndent;
	os << L"</ImportGroup>" << Endl;

	// Properties
	os << L"<PropertyGroup>" << Endl;
	os << IncreaseIndent;
	os << L"<_ProjectFileVersion>10.0.20506.1</_ProjectFileVersion>" << Endl;
	for (RefArray< Configuration >::const_iterator i = configurations.begin(); i != configurations.end(); ++i)
	{
		const Configuration* configuration = *i;
		T_ASSERT (configuration);

		std::wstring name = configuration->getName();
		std::wstring projectName = m_targetPrefixes[int(configuration->getTargetFormat())] + project->getName();

		if (configuration->getTargetProfile() == Configuration::TpDebug)
			projectName += L"_d";

		os << L"<IntDir Condition=\"'$(Configuration)|$(Platform)'=='" << name << L"|" << m_platform << L"'\">$(Configuration)\\</IntDir>" << Endl;
		os << L"<OutDir Condition=\"'$(Configuration)|$(Platform)'=='" << name << L"|" << m_platform << L"'\">$(SolutionDir)$(Configuration)\\</OutDir>" << Endl;
		os << L"<TargetName Condition=\"'$(Configuration)|$(Platform)'=='" << name << L"|" << m_platform << L"'\">" << projectName << L"</TargetName>" << Endl;
		os << L"<TargetExt Condition=\"'$(Configuration)|$(Platform)'=='" << name << L"|" << m_platform << L"'\">" << m_targetExts[int(configuration->getTargetFormat())] << L"</TargetExt>" << Endl;

		switch (configuration->getTargetFormat())
		{
		case Configuration::TfStaticLibrary:
			break;

		case Configuration::TfSharedLibrary:
			if (configuration->getTargetProfile() == Configuration::TpRelease)
				os << L"<LinkIncremental Condition=\"'$(Configuration)|$(Platform)'=='" << name << L"|" << m_platform << L"'\">false</LinkIncremental>" << Endl;
			else
				os << L"<LinkIncremental Condition=\"'$(Configuration)|$(Platform)'=='" << name << L"|" << m_platform << L"'\">true</LinkIncremental>" << Endl;
			os << L"<EmbedManifest Condition=\"'$(Configuration)|$(Platform)'=='" << name << L"|" << m_platform << L"'\">true</EmbedManifest>" << Endl;
			break;

		case Configuration::TfExecutable:
		case Configuration::TfExecutableConsole:
			if (configuration->getTargetProfile() == Configuration::TpRelease)
				os << L"<LinkIncremental Condition=\"'$(Configuration)|$(Platform)'=='" << name << L"|" << m_platform << L"'\">false</LinkIncremental>" << Endl;
			else
				os << L"<LinkIncremental Condition=\"'$(Configuration)|$(Platform)'=='" << name << L"|" << m_platform << L"'\">true</LinkIncremental>" << Endl;
			os << L"<EmbedManifest Condition=\"'$(Configuration)|$(Platform)'=='" << name << L"|" << m_platform << L"'\">true</EmbedManifest>" << Endl;
			break;
		}
	}
	os << DecreaseIndent;
	os << L"</PropertyGroup>" << Endl;

	// Build definitions.
	for (RefArray< Configuration >::const_iterator i = configurations.begin(); i != configurations.end(); ++i)
	{
		Ref< const Configuration > configuration = *i;
		std::wstring name = configuration->getName();

		os << L"<ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='" << name << L"|" << m_platform << L"'\">" << Endl;
		os << IncreaseIndent;

		int32_t format = int32_t(configuration->getTargetFormat());

		const RefArray< SolutionBuilderMsvcVCXDefinition >& buildDefinitions = (configuration->getTargetProfile() == Configuration::TpDebug) ? m_buildDefinitionsDebug[format] : m_buildDefinitionsRelease[format];
		for (RefArray< SolutionBuilderMsvcVCXDefinition >::const_iterator j = buildDefinitions.begin(); j != buildDefinitions.end(); ++j)
		{
			(*j)->generate(
				context,
				solution,
				project,
				configuration,
				os
			);
		}

		os << DecreaseIndent;
		os << L"</ItemDefinitionGroup>" << Endl;
	}

	// Collect all files.
	std::vector< std::pair< std::wstring, Path > > files;
	const RefArray< ProjectItem >& items = project->getItems();
	for (RefArray< ProjectItem >::const_iterator i = items.begin(); i != items.end(); ++i)
	{
		if (!collectFiles(project, *i, L"", files))
			return false;
	}

	// Create item groups.
	for (RefArray< SolutionBuilderMsvcVCXBuildTool >::const_iterator i = m_buildTools.begin(); i != m_buildTools.end(); ++i)
	{
		os << L"<ItemGroup>" << Endl;
		os << IncreaseIndent;

		for (std::vector< std::pair< std::wstring, Path > >::const_iterator j = files.begin(); j != files.end(); ++j)
		{
			Path itemPath;
			FileSystem::getInstance().getRelativePath(
				FileSystem::getInstance().getAbsolutePath(j->second),
				FileSystem::getInstance().getAbsolutePath(projectPath),
				itemPath
			);
			(*i)->generateProject(
				context,
				solution,
				project,
				j->first,
				itemPath,
				os
			);
		}

		os << DecreaseIndent;
		os << L"</ItemGroup>" << Endl;
	}

	os << L"<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />" << Endl;

	// Dependencies.
	os << L"<ItemGroup>" << Endl;
	os << IncreaseIndent;

	const RefArray< Dependency >& dependencies = project->getDependencies();
	for (RefArray< Dependency >::const_iterator i = dependencies.begin(); i != dependencies.end(); ++i)
	{
		Ref< const ProjectDependency > projectDependency = dynamic_type_cast< const ProjectDependency* >(*i);
		if (projectDependency)
		{
			Ref< Project > dependentProject = projectDependency->getProject();
			T_ASSERT (dependentProject);

			std::wstring dependentProjectPath;
			std::wstring dependentProjectFileName;
			std::wstring dependentProjectGuid;

			if (!getInformation(
				context,
				solution,
				dependentProject,
				dependentProjectPath,
				dependentProjectFileName,
				dependentProjectGuid
			))
				return false;

			Path relativePath;
			FileSystem::getInstance().getRelativePath(
				dependentProjectFileName,
				projectPath,
				relativePath
			);

			os << L"<ProjectReference Include=\"" << systemPath(relativePath.getPathName()) << L"\">" << Endl;
			os << IncreaseIndent;
			os << L"<Project>" << dependentProjectGuid << L"</Project>" << Endl;
			os << DecreaseIndent;
			os << L"</ProjectReference>" << Endl;
		}
	}

	os << DecreaseIndent;
	os << L"</ItemGroup>" << Endl;

	os << DecreaseIndent;
	os << L"</Project>" << Endl;

	os.close();

	if (!buffer.empty())
	{
		Ref< IStream > file = FileSystem::getInstance().open(
			projectFileName,
			traktor::File::FmWrite
		);
		if (!file)
			return false;
		file->write(&buffer[0], int(buffer.size()));
		file->close();
	}

	return true;
}

bool SolutionBuilderMsvcVCXProj::generateFilters(
	GeneratorContext& context,
	Solution* solution,
	Project* project
) const
{
	std::wstring projectPath, projectFileName, projectGuid;
	if (!getInformation(
		context,
		solution,
		project,
		projectPath,
		projectFileName,
		projectGuid
	))
		return false;

	if (!FileSystem::getInstance().makeDirectory(projectPath))
		return false;

	traktor::log::info << L"Generating msbuild filters \"" << projectFileName << L".filters\"" << Endl;

	context.set(L"PROJECT_PLATFORM", m_platform);
	context.set(L"PROJECT_NAME", project->getName());
	context.set(L"PROJECT_PATH", projectPath);
	context.set(L"PROJECT_FILENAME", projectFileName);
	context.set(L"PROJECT_GUID", projectGuid);

	std::vector< uint8_t > buffer;
	buffer.reserve(40000);

	DynamicMemoryStream bufferStream(buffer, false, true);
	FileOutputStream os(&bufferStream, new AnsiEncoding());

	os << L"<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">" << Endl;
	os << IncreaseIndent;

	// Collect all files.
	std::vector< std::pair< std::wstring, Path > > files;
	const RefArray< ProjectItem >& items = project->getItems();
	for (RefArray< ProjectItem >::const_iterator i = items.begin(); i != items.end(); ++i)
	{
		if (!collectFiles(project, *i, L"", files))
			return false;
	}

	// Create item groups.
	for (RefArray< SolutionBuilderMsvcVCXBuildTool >::const_iterator i = m_buildTools.begin(); i != m_buildTools.end(); ++i)
	{
		os << L"<ItemGroup>" << Endl;
		os << IncreaseIndent;

		for (std::vector< std::pair< std::wstring, Path > >::const_iterator j = files.begin(); j != files.end(); ++j)
		{
			Path itemPath;
			FileSystem::getInstance().getRelativePath(
				FileSystem::getInstance().getAbsolutePath(j->second),
				FileSystem::getInstance().getAbsolutePath(projectPath),
				itemPath
			);
			(*i)->generateFilter(
				context,
				solution,
				project,
				j->first,
				itemPath,
				os
			);
		}

		os << DecreaseIndent;
		os << L"</ItemGroup>" << Endl;
	}

	// Create filter guids.
	std::set< std::wstring > filters;
	for (std::vector< std::pair< std::wstring, Path > >::const_iterator i = files.begin(); i != files.end(); ++i)
	{
		if (i->first != L"")
			filters.insert(i->first);
	}
	if (!filters.empty())
	{
		for (std::set< std::wstring >::const_iterator i = filters.begin(); i != filters.end(); ++i)
		{
			Guid filterGuid = Guid::create();

			os << L"<ItemGroup>" << Endl;
			os << IncreaseIndent;

			os << L"<Filter Include=\"" << *i << L"\">" << Endl;
			os << L"<UniqueIdentifier>" << filterGuid.format() << L"</UniqueIdentifier>" << Endl;
			os << L"</Filter>" << Endl;

			os << DecreaseIndent;
			os << L"</ItemGroup>" << Endl;
		}
	}

	os << DecreaseIndent;
	os << L"</Project>" << Endl;

	os.close();

	if (!buffer.empty())
	{
		Ref< IStream > file = FileSystem::getInstance().open(
			projectFileName + L".filters",
			traktor::File::FmWrite
		);
		if (!file)
			return false;
		file->write(&buffer[0], int(buffer.size()));
		file->close();
	}

	return true;
}

bool SolutionBuilderMsvcVCXProj::collectFiles(
	Project* project,
	ProjectItem* item,
	const std::wstring& filterPath,
	std::vector< std::pair< std::wstring, Path > >& outFiles
) const
{
	Ref< Filter > filter = dynamic_type_cast< Filter* >(item);
	if (filter)
	{
		std::wstring childFilterPath;

		if (!filterPath.empty())
			childFilterPath = filterPath + L"\\" + filter->getName();
		else
			childFilterPath = filter->getName();

		const RefArray< ProjectItem >& items = item->getItems();
		for (RefArray< ProjectItem >::const_iterator i = items.begin(); i != items.end(); ++i)
			collectFiles(
				project,
				*i,
				childFilterPath,
				outFiles
			);
	}

	Ref< ::File > file = dynamic_type_cast< ::File* >(item);
	if (file)
	{
		std::set< Path > systemFiles;
		file->getSystemFiles(project->getSourcePath(), systemFiles);
		for (std::set< Path >::iterator i = systemFiles.begin(); i != systemFiles.end(); ++i)
			outFiles.push_back(std::make_pair(filterPath, *i));
	}

	return true;
}

void SolutionBuilderMsvcVCXProj::findDefinitions(
	GeneratorContext& context,
	Solution* solution,
	Project* project,
	const RefArray< ProjectItem >& items
) const
{
	Path rootPath = FileSystem::getInstance().getAbsolutePath(context.get(L"PROJECT_PATH"));

	for (RefArray< ProjectItem >::const_iterator i = items.begin(); i != items.end(); ++i)
	{
		if (const ::File* file = dynamic_type_cast< const ::File* >(*i))
		{
			std::set< Path > systemFiles;
			file->getSystemFiles(project->getSourcePath(), systemFiles);
			for (std::set< Path >::iterator j = systemFiles.begin(); j != systemFiles.end(); ++j)
			{
				if (compareIgnoreCase< std::wstring >(j->getExtension(), L"def") == 0)
				{
					Path relativePath;
					FileSystem::getInstance().getRelativePath(
						*j,
						rootPath,
						relativePath
						);
					context.set(L"MODULE_DEFINITION_FILE", relativePath.getPathName());
				}
			}
		}
		findDefinitions(context, solution, project, (*i)->getItems());
	}
}
