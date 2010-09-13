#ifndef traktor_flash_ActionObject_H
#define traktor_flash_ActionObject_H

#include <map>
#include <string>
#include "Core/Object.h"
#include "Flash/Action/ActionTypes.h"
#include "Flash/Action/ActionValue.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_FLASH_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace flash
	{

class ActionFunction;
class ActionValue;

/*! \brief ActionScript object.
 * \ingroup Flash
 */
class T_DLLCLASS ActionObject : public Object
{
	T_RTTI_CLASS;

public:
	typedef std::map< std::wstring, ActionValue > member_map_t;
	typedef std::map< std::wstring, std::pair< Ref< ActionFunction >, Ref< ActionFunction > > > property_map_t;

	ActionObject();

	ActionObject(ActionObject* prototype);
	
	virtual void release() const;

	virtual void addInterface(ActionObject* intrface);

	virtual void setMember(const std::wstring& memberName, const ActionValue& memberValue);

	virtual bool getMember(const std::wstring& memberName, ActionValue& outMemberValue) const;

	virtual bool deleteMember(const std::wstring& memberName);

	virtual void addProperty(const std::wstring& propertyName, ActionFunction* propertyGet, ActionFunction* propertySet);

	virtual bool getPropertyGet(const std::wstring& propertyName, Ref< ActionFunction >& outPropertyGet) const;

	virtual bool getPropertySet(const std::wstring& propertyName, Ref< ActionFunction >& outPropertySet) const;

	virtual const property_map_t& getProperties() const;

	virtual avm_number_t valueOf() const;

	virtual std::wstring toString() const;

	void setReadOnly();

	bool getLocalMember(const std::wstring& memberName, ActionValue& outMemberValue) const;

	bool hasOwnProperty(const std::wstring& propertyName) const;

	bool getLocalPropertyGet(const std::wstring& propertyName, Ref< ActionFunction >& outPropertyGet) const;

	bool getLocalPropertySet(const std::wstring& propertyName, Ref< ActionFunction >& outPropertySet) const;

private:
	bool m_readOnly;
	mutable member_map_t m_members;
	property_map_t m_properties;
	
	static int32_t ms_traceTag;
	mutable int32_t m_tracedTag;
	mutable bool m_cycle;
	
	/*
	int32_t traceCycles(const ActionObject* root) const;
	
	void resetCycles(const ActionObject* root) const;
	*/
};

	}
}

#endif	// traktor_flash_ActionObject_H
