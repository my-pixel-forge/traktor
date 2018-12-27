/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#ifndef traktor_mesh_MaterialShaderGenerator_H
#define traktor_mesh_MaterialShaderGenerator_H

#include "Core/Object.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_MESH_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace db
	{

class Database;

	}
	
	namespace editor
	{
	
class IPipelineDepends;
	
	}

	namespace render
	{

class ShaderGraph;

	}

	namespace model
	{

class Material;

	}

	namespace mesh
	{

class T_DLLCLASS MaterialShaderGenerator : public Object
{
	T_RTTI_CLASS;

public:
	Ref< render::ShaderGraph > generate(
		db::Database* database,
		const model::Material& material,
		const Guid& materialTemplate,
		const std::map< std::wstring, Guid >& textures,
		bool vertexColor
	) const;
	
	void addDependencies(editor::IPipelineDepends* pipelineDepends);
};

	}
}

#endif	// traktor_mesh_MaterialShaderGenerator_H
