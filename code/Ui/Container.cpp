/*
 * TRAKTOR
 * Copyright (c) 2022 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Core/Log/Log.h"
#include "Ui/Application.h"
#include "Ui/Container.h"
#include "Ui/Layout.h"
#include "Ui/StyleSheet.h"
#include "Ui/Itf/IContainer.h"

namespace traktor
{
	namespace ui
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.ui.Container", Container, Widget)

bool Container::create(Widget* parent, int style, Layout* layout)
{
	m_layout = layout;

	if (!m_widget)
	{
		IContainer* container = Application::getInstance()->getWidgetFactory()->createContainer(this);
		if (!container)
		{
			log::error << L"Failed to create native widget peer (Container)" << Endl;
			return false;
		}

		if (!container->create(parent->getIWidget(), style))
		{
			container->destroy();
			return false;
		}

		m_widget = container;
	}

	addEventHandler< SizeEvent >(this, &Container::eventSize);

	if ((style & WsNoCanvas) == 0)
		addEventHandler< PaintEvent >(this, &Container::eventPaint);

	return Widget::create(parent);
}

void Container::fit(uint32_t axis)
{
	if (!m_layout || axis == 0)
		return;

	// First fit child containers.
	for (Ref< Widget > child = getFirstChild(); child; child = child->getNextSibling())
	{
		if (auto childContainer = dynamic_type_cast< Container* >(child))
			childContainer->fit(axis);
	}

	// Use layout to calculate size of container.
	Size inner = getInnerRect().getSize();
	Size bounds;
	if (m_layout->fit(this, inner, bounds))
	{
		Rect outer = getRect();
		Size nc = outer.getSize() - inner;

		if ((axis & Horizontal) != 0)
			outer.setWidth(bounds.cx + nc.cx);
		if ((axis & Vertical) != 0)
			outer.setHeight(bounds.cy + nc.cy);

		setRect(outer);
	}
}

void Container::update(const Rect* rc, bool immediate)
{
	if (m_layout)
		m_layout->update(this);

	Widget::update(rc, immediate);
}

Size Container::getMinimumSize() const
{
	return Widget::getMinimumSize();
}

Size Container::getPreferredSize(const Size& hint) const
{
	if (m_layout)
	{
		// Use layout to fit hinted size.
		Size bounds;
		if (m_layout->fit(const_cast< Container* >(this), hint, bounds))
			return bounds;
	}
	return Widget::getPreferredSize(hint);
}

Size Container::getMaximumSize() const
{
	return Widget::getMaximumSize();
}

Ref< Layout > Container::getLayout() const
{
	return m_layout;
}

void Container::setLayout(Layout* layout)
{
	m_layout = layout;
}

void Container::eventSize(SizeEvent* event)
{
	update(nullptr, false);
}

void Container::eventPaint(PaintEvent* event)
{
	Canvas& canvas = event->getCanvas();
	const StyleSheet* ss = getStyleSheet();

	canvas.setBackground(ss->getColor(this, L"background-color"));
	canvas.fillRect(event->getUpdateRect());
}

	}
}
