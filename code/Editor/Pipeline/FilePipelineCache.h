#pragma once

#include "Editor/IPipelineCache.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace editor
	{

class T_DLLCLASS FilePipelineCache : public IPipelineCache
{
	T_RTTI_CLASS;

public:
	FilePipelineCache();

	virtual bool create(const PropertyGroup* settings) override final;

	virtual void destroy() override final;

	virtual Ref< IStream > get(const Guid& guid, const PipelineDependencyHash& hash) override final;

	virtual Ref< IStream > put(const Guid& guid, const PipelineDependencyHash& hash) override final;

	virtual bool commit(const Guid& guid, const PipelineDependencyHash& hash) override final;

	virtual void getInformation(OutputStream& os) const override final;

private:
	bool m_accessRead;
	bool m_accessWrite;
	std::wstring m_path;
};

	}
}

