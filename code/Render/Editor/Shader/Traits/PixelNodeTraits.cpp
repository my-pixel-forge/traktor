#include "Render/Editor/Shader/Nodes.h"
#include "Render/Editor/Shader/ShaderGraph.h"
#include "Render/Editor/Shader/Traits/PixelNodeTraits.h"

namespace traktor
{
	namespace render
	{
		namespace
		{

int32_t getInputPinIndex(const Node* node, const InputPin* inputPin)
{
	int32_t inputPinCount = node->getInputPinCount();
	for (int32_t i = 0; i < inputPinCount; ++i)
	{
		if (node->getInputPin(i) == inputPin)
			return i;
	}
	T_FATAL_ERROR;
	return -1;
}

		}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.render.PixelNodeTraits", 0, PixelNodeTraits, INodeTraits)

TypeInfoSet PixelNodeTraits::getNodeTypes() const
{
	return makeTypeInfoSet< PixelOutput >();
}

bool PixelNodeTraits::isRoot(const ShaderGraph* shaderGraph, const Node* node) const
{
	return true;
}

PinType PixelNodeTraits::getOutputPinType(
	const ShaderGraph* shaderGraph,
	const Node* node,
	const OutputPin* outputPin,
	const PinType* inputPinTypes
) const
{
	return PinType::Void;
}

PinType PixelNodeTraits::getInputPinType(
	const ShaderGraph* shaderGraph,
	const Node* node,
	const InputPin* inputPin,
	const PinType* inputPinTypes,
	const PinType* outputPinTypes
) const
{
	const PixelOutput* pixelOutputNode = mandatory_non_null_type_cast< const PixelOutput* >(node);
	RenderState rs = pixelOutputNode->getRenderState();

	const OutputPin* statePin = shaderGraph->findSourcePin(pixelOutputNode->getInputPin(5));
	if (statePin)
	{
		const PixelState* state = dynamic_type_cast< const PixelState* >(statePin->getNode());
		if (state)
			rs = state->getRenderState();
	}

	if (!rs.blendEnable && !rs.alphaTestEnable)
	{
		if (rs.colorWriteMask & CwAlpha)
			return PinType::Scalar4;
		else if (rs.colorWriteMask & CwBlue)
			return PinType::Scalar3;
		else if (rs.colorWriteMask & CwGreen)
			return PinType::Scalar2;
		else if (rs.colorWriteMask & CwRed)
			return PinType::Scalar1;
		else
			return PinType::Void;
	}
	else
		return PinType::Scalar4;
}

int32_t PixelNodeTraits::getInputPinGroup(
	const ShaderGraph* shaderGraph,
	const Node* node,
	const InputPin* inputPin
) const
{
	return getInputPinIndex(node, inputPin);
}

bool PixelNodeTraits::evaluatePartial(
	const ShaderGraph* shaderGraph,
	const Node* node,
	const OutputPin* nodeOutputPin,
	const Constant* inputConstants,
	Constant& outputConstant
) const
{
	return false;
}

bool PixelNodeTraits::evaluatePartial(
	const ShaderGraph* shaderGraph,
	const Node* node,
	const OutputPin* nodeOutputPin,
	const OutputPin** inputOutputPins,
	const Constant* inputConstants,
	const OutputPin*& foldOutputPin
) const
{
	return false;
}

PinOrderType PixelNodeTraits::evaluateOrder(
	const ShaderGraph* shaderGraph,
	const Node* node,
	const OutputPin* nodeOutputPin,
	const PinOrderType* inputPinOrders,
	bool frequentAsLinear
) const
{
	return PotConstant;
}

	}
}
