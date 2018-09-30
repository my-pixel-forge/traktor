/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2017 Doctor Entertainment AB. All Rights Reserved.
================================================================================================
*/
#include "Ui/Application.h"
#include "Ui/Canvas.h"
#include "Ui/Sequencer/Sequence.h"
#include "Ui/Sequencer/Tick.h"

namespace traktor
{
	namespace ui
	{
		namespace
		{

const int c_sequenceHeight = 40;

		}

T_IMPLEMENT_RTTI_CLASS(L"traktor.ui.Tick", Tick, Key)

Tick::Tick(int time, bool movable)
:	m_time(time)
,	m_movable(movable)
{
}

void Tick::setTime(int time)
{
	m_time = time;
}

int Tick::getTime() const
{
	return m_time;
}

void Tick::move(int offset)
{
	if (m_movable)
		m_time += offset;
}

void Tick::getRect(const Sequence* sequence, const Rect& rcClient, Rect& outRect) const
{
	int32_t sequenceHeight = dpi96(c_sequenceHeight);
	int32_t x = sequence->clientFromTime(m_time);
	outRect.left = x - 3;
	outRect.top = rcClient.top + 2;
	outRect.right = x + 4;
	outRect.bottom = rcClient.top + sequenceHeight - 3;
}

void Tick::paint(ui::Canvas& canvas, const Sequence* sequence, const Rect& rcClient, int scrollOffset)
{
	int32_t sequenceHeight = dpi96(c_sequenceHeight);
	int32_t x = sequence->clientFromTime(m_time) - scrollOffset;

	Rect rc(rcClient.left + x - 3, rcClient.top + 2, rcClient.left + x + 4, rcClient.top + sequenceHeight - 3);

	if (sequence->getSelectedKey() != this)
	{
		canvas.setForeground(Color4ub(220, 220, 255));
		canvas.setBackground(Color4ub(180, 180, 230));
	}
	else
	{
		canvas.setForeground(Color4ub(180, 180, 255));
		canvas.setBackground(Color4ub(160, 160, 255));
	}

	canvas.fillGradientRect(rc);

	canvas.setForeground(Color4ub(0, 0, 0, 128));
	canvas.drawRect(rc);

	if (sequence->getSelectedKey() == this)
	{
		canvas.setForeground(Color4ub(255, 255, 255));
		canvas.drawRect(rc.inflate(2, 2));
	}
}

	}
}
