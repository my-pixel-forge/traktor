#pragma once

#include <map>
#include "Core/Ref.h"
#include "Core/Object.h"
#include "Core/Math/Matrix33.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_SVG_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace xml
	{

class Document;
class Element;

	}

	namespace svg
	{

class Shape;
class Style;
class Gradient;

/*! SVG parser.
 * \ingroup SVG
 */
class T_DLLCLASS Parser : public Object
{
	T_RTTI_CLASS;

public:
	Ref< Shape > parse(xml::Document* doc);

private:
	std::map< std::wstring, Ref< Gradient > > m_gradients;

	Ref< Shape > traverse(xml::Element* elm);

	Ref< Shape > parseDocument(xml::Element* elm);

	Ref< Shape > parseGroup(xml::Element* elm);

	Ref< Shape > parseCircle(xml::Element* elm);

	Ref< Shape > parseRect(xml::Element* elm);

	Ref< Shape > parsePolygon(xml::Element* elm);

	Ref< Shape > parsePolyLine(xml::Element* elm);

	Ref< Shape > parsePath(xml::Element* elm);

	Ref< Shape > parseText(xml::Element* elm);

	void parseDefs(xml::Element* elm);

	Ref< Style > parseStyle(xml::Element* elm);

	Matrix33 parseTransform(xml::Element* elm);

	float parseAttr(xml::Element* elm, const std::wstring& attrName, float defValue = 0.0f) const;
};

	}
}

