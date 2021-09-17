#pragma once

#include <string>
#include <tuple>
#include "Core/Config.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_AVALANCHE_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

class IStream;

	namespace avalanche
	{

class T_DLLCLASS Key
{
public:
	Key();

	Key(uint32_t k0, uint32_t k1, uint32_t k2, uint32_t k3);

	bool valid() const;

	std::wstring format() const;

	static Key read(IStream* stream);

	bool write(IStream* stream) const;

	bool operator == (const Key& rh) const;

	bool operator < (const Key& rh) const;

	bool operator > (const Key& rh) const;

private:
	std::tuple< uint32_t, uint32_t, uint32_t, uint32_t > m_kv;
};

	}
}
