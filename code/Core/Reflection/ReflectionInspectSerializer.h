#ifndef traktor_ReflectionInspectSerializer_H
#define traktor_ReflectionInspectSerializer_H

#include "Core/RefArray.h"
#include "Core/Serialization/Serializer.h"

namespace traktor
{

class ReflectionMember;
class RfmCompound;

/*! \brief Internal reflection inspection serializer.
 * \ingroup Core
 */
class ReflectionInspectSerializer : public Serializer
{
	T_RTTI_CLASS;

public:
	ReflectionInspectSerializer(RfmCompound* compound);

	virtual Direction getDirection() const;

	virtual bool operator >> (const Member< bool >& m);

	virtual bool operator >> (const Member< int8_t >& m);

	virtual bool operator >> (const Member< uint8_t >& m);

	virtual bool operator >> (const Member< int16_t >& m);

	virtual bool operator >> (const Member< uint16_t >& m);

	virtual bool operator >> (const Member< int32_t >& m);

	virtual bool operator >> (const Member< uint32_t >& m);

	virtual bool operator >> (const Member< int64_t >& m);

	virtual bool operator >> (const Member< uint64_t >& m);

	virtual bool operator >> (const Member< float >& m);

	virtual bool operator >> (const Member< double >& m);

	virtual bool operator >> (const Member< std::string >& m);

	virtual bool operator >> (const Member< std::wstring >& m);

	virtual bool operator >> (const Member< Guid >& m);

	virtual bool operator >> (const Member< Path >& m);

	virtual bool operator >> (const Member< Color4ub >& m);

	virtual bool operator >> (const Member< Color4f >& m);

	virtual bool operator >> (const Member< Scalar >& m);

	virtual bool operator >> (const Member< Vector2 >& m);

	virtual bool operator >> (const Member< Vector4 >& m);

	virtual bool operator >> (const Member< Matrix33 >& m);

	virtual bool operator >> (const Member< Matrix44 >& m);

	virtual bool operator >> (const Member< Quaternion >& m);

	virtual bool operator >> (const Member< ISerializable* >& m);

	virtual bool operator >> (const Member< void* >& m);

	virtual bool operator >> (const MemberArray& m);

	virtual bool operator >> (const MemberComplex& m);

	virtual bool operator >> (const MemberEnumBase& m);

private:
	friend class Reflection;

	Ref< RfmCompound > m_compoundMember;

	bool addMember(ReflectionMember* member);
};

}

#endif	// traktor_ReflectionInspectSerializer_H
