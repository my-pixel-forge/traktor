#include "Ai/NavMeshResource.h"

namespace traktor
{
	namespace ai
	{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.ai.NavMeshResource", 0, NavMeshResource, ISerializable)

bool NavMeshResource::serialize(ISerializer& s)
{
	return true;
}

	}
}
