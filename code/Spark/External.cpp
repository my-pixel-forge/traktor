#include "Core/Serialization/ISerializer.h"
#include "Resource/IResourceManager.h"
#include "Resource/Member.h"
#include "Spark/External.h"

namespace traktor
{
	namespace spark
	{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"traktor.spark.External", 0, External, Character)

Ref< CharacterInstance > External::createInstance(const CharacterInstance* parent, resource::IResourceManager* resourceManager, sound::ISoundPlayer* soundPlayer) const
{
	resource::Proxy< Character > character;
	if (resourceManager->bind(m_reference, character))
		return character->createInstance(parent, resourceManager, soundPlayer);
	else
		return 0;
}

void External::serialize(ISerializer& s)
{
	s >> resource::Member< Character >(L"reference", m_reference);
}

	}
}
