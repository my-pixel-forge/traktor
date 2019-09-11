#pragma once

#include "Core/Containers/AlignedVector.h"
#include "Core/Serialization/ISerializable.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_SPRAY_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace world
	{

class IEntityBuilder;
class IEntityEventData;

	}

	namespace spray
	{

class Sequence;

/*! Trigger sequence persistent data.
 * \ingroup Spray
 */
class T_DLLCLASS SequenceData : public ISerializable
{
	T_RTTI_CLASS;

public:
	struct Key
	{
		float T;
		Ref< world::IEntityEventData > event;

		void serialize(ISerializer& s);
	};

	Ref< Sequence > createSequence(const world::IEntityBuilder* entityBuilder) const;

	virtual void serialize(ISerializer& s) override final;

	const AlignedVector< Key >& getKeys() const { return m_keys; }

	 AlignedVector< Key >& getKeys() { return m_keys; }

private:
	AlignedVector< Key > m_keys;
};

	}
}

