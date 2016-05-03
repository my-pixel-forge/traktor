#ifndef traktor_ui_custom_ExternalNodeShape_H
#define traktor_ui_custom_ExternalNodeShape_H

#include "Ui/Custom/Graph/NodeShape.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_UI_CUSTOM_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{
	namespace ui
	{

class Bitmap;

		namespace custom
		{

class GraphControl;

/*! \brief External node shape.
 * \ingroup UIC
 */
class T_DLLCLASS ExternalNodeShape : public NodeShape
{
	T_RTTI_CLASS;

public:
	ExternalNodeShape(GraphControl* graphControl);

	virtual Point getPinPosition(const Node* node, const Pin* pin) T_OVERRIDE;

	virtual Ref< Pin > getPinAt(const Node* node, const Point& pt) T_OVERRIDE;

	virtual void paint(const Node* node, const PaintSettings* settings, Canvas* canvas, const Size& offset) T_OVERRIDE;

	virtual Size calculateSize(const Node* node) T_OVERRIDE;

private:
	Ref< GraphControl > m_graphControl;
	Ref< Bitmap > m_imageNode;
	Ref< Bitmap > m_imagePin;
};

		}
	}
}

#endif	// traktor_ui_custom_ExternalNodeShape_H
