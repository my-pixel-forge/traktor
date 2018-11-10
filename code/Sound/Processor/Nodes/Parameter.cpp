#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Sound/ISoundBuffer.h"
#include "Sound/Processor/Nodes/Parameter.h"

namespace traktor
{
	namespace sound
	{
		namespace
		{

const ImmutableNode::OutputPinDesc c_Parameter_o[] =
{
	{ L"Output", NptScalar },
	{ 0 }
};

class ParameterCursor : public RefCountImpl< ISoundBufferCursor >
{
public:
	handle_t m_id;
	float m_value;

	ParameterCursor(handle_t id)
	:	m_id(id)
	,	m_value(0.0f)
	{
	}

	virtual void setParameter(handle_t id, float parameter) T_OVERRIDE T_FINAL
	{
		if (id == m_id)
			m_value = parameter;
	}

	virtual void disableRepeat() T_OVERRIDE T_FINAL {}

	virtual void reset() T_OVERRIDE T_FINAL {}
};

		}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.sound.Parameter", 0, Parameter, ImmutableNode)

Parameter::Parameter()
:	ImmutableNode(nullptr, c_Parameter_o)
{
}

bool Parameter::bind(resource::IResourceManager* resourceManager)
{
	return true;
}

Ref< ISoundBufferCursor > Parameter::createCursor() const 
{
	return new ParameterCursor(getParameterHandle(m_name));
}

bool Parameter::getScalar(ISoundBufferCursor* cursor, const GraphEvaluator* evaluator, float& outParameter) const
{
	ParameterCursor* parameterCursor = static_cast< ParameterCursor* >(cursor);
	outParameter = parameterCursor->m_value;
	return true;
}

bool Parameter::getBlock(ISoundBufferCursor* cursor, const GraphEvaluator* evaluator, const ISoundMixer* mixer, SoundBlock& outBlock) const
{
	return false;
}

void Parameter::serialize(ISerializer& s)
{
	ImmutableNode::serialize(s);

	s >> Member< std::wstring >(L"name", m_name);
}

	}
}