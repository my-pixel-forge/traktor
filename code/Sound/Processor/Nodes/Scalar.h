#pragma once

#include "Sound/Processor/ImmutableNode.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_SOUND_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace sound
	{

class T_DLLCLASS Scalar : public ImmutableNode
{
	T_RTTI_CLASS;

public:
	Scalar();

	virtual bool bind(resource::IResourceManager* resourceManager) override final;

	virtual Ref< ISoundBufferCursor > createCursor() const override final;

	virtual bool getScalar(ISoundBufferCursor* cursor, const GraphEvaluator* evaluator, float& outScalar) const override final;
	
	virtual bool getBlock(ISoundBufferCursor* cursor, const GraphEvaluator* evaluator, const ISoundMixer* mixer, SoundBlock& outBlock) const override final;

	virtual void serialize(ISerializer& s) override final;

	float getValue() const { return m_value; }

private:
	float m_value;
};

	}
}
