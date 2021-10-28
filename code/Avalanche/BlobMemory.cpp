#include "Avalanche/BlobMemory.h"
#include "Core/Io/ChunkMemory.h"
#include "Core/Io/ChunkMemoryStream.h"

namespace traktor
{
	namespace avalanche
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.avalanche.BlobMemory", BlobMemory, Object)

BlobMemory::BlobMemory()
:	m_memory(new ChunkMemory())
,	m_lastAccessed(DateTime::now())
{
}

int64_t BlobMemory::size() const
{
	return m_memory->size();
}

Ref< IStream > BlobMemory::append()
{
	Ref< ChunkMemoryStream > stream = new ChunkMemoryStream(m_memory, false, true);
	stream->seek(IStream::SeekEnd, 0);
	return stream;
}

Ref< IStream > BlobMemory::read() const
{
	m_lastAccessed = DateTime::now();
	return new ChunkMemoryStream(m_memory, true, false);
}

bool BlobMemory::remove()
{
	return true;
}

bool BlobMemory::touch()
{
	m_lastAccessed = DateTime::now();
	return true;
}

DateTime BlobMemory::lastAccessed() const
{
	return m_lastAccessed;
}

	}
}
