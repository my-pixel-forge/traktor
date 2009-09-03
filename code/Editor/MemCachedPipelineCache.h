#ifndef traktor_editor_MemCachedPipelineCache_H
#define traktor_editor_MemCachedPipelineCache_H

#include "Core/Heap/Ref.h"
#include "Editor/IPipelineCache.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_EDITOR_EXPORT)
#define T_DLLCLASS T_DLLEXPORT
#else
#define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace net
	{

class TcpSocket;

	}

	namespace editor
	{

class MemCachedProto;

class T_DLLCLASS MemCachedPipelineCache : public IPipelineCache
{
	T_RTTI_CLASS(MemCachedPipelineCache)

public:
	MemCachedPipelineCache();

	virtual ~MemCachedPipelineCache();

	virtual bool create(const Settings* settings);

	virtual Stream* get(const Guid& guid, uint32_t hash) const;

	virtual Stream* put(const Guid& guid, uint32_t hash, Stream* source);

private:
	Ref< net::TcpSocket > m_socket;
	Ref< MemCachedProto > m_proto;
};

	}
}

#endif	// traktor_editor_MemCachedPipelineCache_H
