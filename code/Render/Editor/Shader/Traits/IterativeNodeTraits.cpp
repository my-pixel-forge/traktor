#include <algorithm>
#include "Render/Editor/Shader/Nodes.h"
#include "Render/Editor/Shader/Traits/IterativeNodeTraits.h"

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

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.render.IterativeNodeTraits", 0, IterativeNodeTraits, INodeTraits)

TypeInfoSet IterativeNodeTraits::getNodeTypes() const
{
	TypeInfoSet typeSet;
	typeSet.insert< Iterate >();
	typeSet.insert< Iterate2 >();
	typeSet.insert< Iterate2d >();
	typeSet.insert< Repeat >();
	typeSet.insert< Sum >();
	return typeSet;
}

bool IterativeNodeTraits::isRoot(const ShaderGraph* shaderGraph, const Node* node) const
{
	return false;
}

PinType IterativeNodeTraits::getOutputPinType(
	const ShaderGraph* shaderGraph,
	const Node* node,
	const OutputPin* outputPin,
	const PinType* inputPinTypes
) const
{
	if (
		outputPin->getName() == L"N" ||
		outputPin->getName() == L"X" ||
		outputPin->getName() == L"Y"
	)
		return PntScalar1;
	else if (
		is_a< Iterate >(node) ||
		is_a< Iterate2d >(node) ||
		is_a< Repeat >(node)
	)
	{
		return std::max< PinType >(
			inputPinTypes[0],			// Input
			inputPinTypes[1]			// Initial
		);
	}
	else if (is_a< Iterate2 >(node))
	{
		if (outputPin->getName() == L"Output0")
		{
			return std::max< PinType >(
				inputPinTypes[2],			// Input0
				inputPinTypes[3]			// Initial0
			);
		}
		else if (outputPin->getName() == L"Output1")
		{
			return std::max< PinType >(
				inputPinTypes[4],			// Input1
				inputPinTypes[5]			// Initial1
			);
		}
		else if (outputPin->getName() == L"Output2")
		{
			return std::max< PinType >(
				inputPinTypes[6],			// Input2
				inputPinTypes[7]			// Initial2
			);
		}
		else if (outputPin->getName() == L"Output3")
		{
			return std::max< PinType >(
				inputPinTypes[8],			// Input3
				inputPinTypes[9]			// Initial3
			);
		}
		else
			return PntVoid;
	}
	else
		return inputPinTypes[0];
}

PinType IterativeNodeTraits::getInputPinType(
	const ShaderGraph* shaderGraph,
	const Node* node,
	const InputPin* inputPin,
	const PinType* inputPinTypes,
	const PinType* outputPinTypes
) const
{
	if (inputPin->getName() == L"Condition")
		return PntScalar1;
	else if (is_a< Iterate2 >(node))
	{
		if (inputPin->getName() == L"From" || inputPin->getName() == L"To")
			return PntScalar1;
		else if (inputPin->getName() == L"Input0")
			return outputPinTypes[2];	// Output0
		else if (inputPin->getName() == L"Input1")
			return outputPinTypes[3];	// Output1
		else if (inputPin->getName() == L"Input2")
			return outputPinTypes[4];	// Output2
		else if (inputPin->getName() == L"Input3")
			return outputPinTypes[5];	// Output3
		else
			return PntVoid;		
	}
	else
		return outputPinTypes[1];	// Output
}

int32_t IterativeNodeTraits::getInputPinGroup(
	const ShaderGraph* shaderGraph,
	const Node* node,
	const InputPin* inputPin
) const
{
	return getInputPinIndex(node, inputPin);
}

bool IterativeNodeTraits::evaluatePartial(
	const ShaderGraph* shaderGraph,
	const Node* node,
	const OutputPin* nodeOutputPin,
	const Constant* inputConstants,
	Constant& outputConstant
) const
{
	return false;
}

bool IterativeNodeTraits::evaluatePartial(
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

PinOrderType IterativeNodeTraits::evaluateOrder(
	const ShaderGraph* shaderGraph,
	const Node* node,
	const OutputPin* nodeOutputPin,
	const PinOrderType* inputPinOrders,
	bool frequentAsLinear
) const
{
	if (
		nodeOutputPin->getName() == L"N" ||
		nodeOutputPin->getName() == L"X" ||
		nodeOutputPin->getName() == L"Y"
	)
		return PotConstant;
	else if (
		is_a< Iterate >(node) ||
		is_a< Iterate2d >(node) ||
		is_a< Repeat >(node)
	)
		return pinOrderMax(
			inputPinOrders[0],	// Input
			inputPinOrders[1]	// Initial
		);
	else if (is_a< Iterate2 >(node))
	{
		if (nodeOutputPin->getName() == L"Output0")
			return pinOrderMax(
				inputPinOrders[2],	// Input0
				inputPinOrders[3]	// Initial0
			);
		else if (nodeOutputPin->getName() == L"Output1")
			return pinOrderMax(
				inputPinOrders[4],	// Input1
				inputPinOrders[5]	// Initial1
			);
		else if (nodeOutputPin->getName() == L"Output2")
			return pinOrderMax(
				inputPinOrders[6],	// Input2
				inputPinOrders[7]	// Initial2
			);
		else if (nodeOutputPin->getName() == L"Output3")
			return pinOrderMax(
				inputPinOrders[8],	// Input3
				inputPinOrders[9]	// Initial3
			);
		else
			return PotConstant;
	}
	else	// Sum
		return inputPinOrders[0];
}

	}
}
