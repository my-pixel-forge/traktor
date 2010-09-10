#include <iostream>
#include "Core/Platform.h"
#include "Core/Memory/TrackAllocator.h"
#include "Core/Thread/Acquire.h"

namespace traktor
{

TrackAllocator::TrackAllocator(IAllocator* systemAllocator)
:	m_systemAllocator(systemAllocator)
{
}

TrackAllocator::~TrackAllocator()
{
	if (!m_aliveBlocks.empty())
	{
		std::wcout << L"Memory leak detected, following allocation(s) not freed:" << std::endl;
		for (std::map< void*, Block >::const_iterator i = m_aliveBlocks.begin(); i != m_aliveBlocks.end(); ++i)
		{
			std::wcout << L"0x" << i->first << L", " << i->second.size << L" byte(s), tag \"" << i->second.tag << L"\"" << std::endl;
			for (int j = 0; j < sizeof_array(i->second.at); ++j)
				std::wcout << L"   " << j << L": 0x" << i->second.at[j] << std::endl;
		}
	}
	else
		std::wcout << L"No memory leaks! Good work!" << std::endl;
}

void* TrackAllocator::alloc(size_t size, size_t align, const char* const tag)
{
	T_ANONYMOUS_VAR(Acquire< Semaphore >)(m_lock);

	void* ptr = m_systemAllocator->alloc(size, align, tag);
	if (!ptr)
		return 0;

	Block block;
	block.tag = tag;
	block.size = size;
	block.at[0] =
	block.at[1] =
	block.at[2] =
	block.at[3] = 0;

#if defined(_WIN32) && !defined(_WIN64) && !defined(WINCE)

	uint32_t at_0 = 0, at_1 = 0;
	uint32_t at_2 = 0, at_3 = 0;
	__asm
	{
		mov ecx, [ebp]
		mov eax, [ecx + 4]
		mov at_0, eax
	}
	block.at[0] = (void*)(at_0 - 6);
	block.at[1] = (void*)at_1;
	block.at[2] = (void*)at_2;
	block.at[3] = (void*)at_3;

#endif

	m_aliveBlocks.insert(std::make_pair(ptr, block));
	m_allocCount[block.at[0]]++;

	return ptr;
}

void TrackAllocator::free(void* ptr)
{
	T_ANONYMOUS_VAR(Acquire< Semaphore >)(m_lock);

	std::map< void*, Block >::iterator i = m_aliveBlocks.find(ptr);
	T_ASSERT (i != m_aliveBlocks.end());

	if (i != m_aliveBlocks.end())
		m_aliveBlocks.erase(i);

	m_systemAllocator->free(ptr);
}

}
