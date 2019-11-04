#pragma once

#import <Cocoa/Cocoa.h>
#import "Ui/Cocoa/NSWindowDelegateProxy.h"

#include <map>
#include "Ui/Itf/IFontMetric.h"
#include "Ui/Itf/IForm.h"

namespace traktor
{
	namespace ui
	{

class EventSubject;

class FormCocoa
:	public IForm
,	public IFontMetric
,	public INSWindowEventsCallback
{
public:
	FormCocoa(EventSubject* owner);

	// IForm implementation

	virtual bool create(IWidget* parent, const std::wstring& text, int width, int height, int style) override final;

	virtual void setIcon(ISystemBitmap* icon) override final;

	virtual void maximize() override final;

	virtual void minimize() override final;

	virtual void restore() override final;

	virtual bool isMaximized() const override final;

	virtual bool isMinimized() const override final;

	virtual void hideProgress() override final;

	virtual void showProgress(int32_t current, int32_t total) override final;

	// IWidget implementation

	virtual void destroy() override final;

	virtual void setParent(IWidget* parent) override final;

	virtual void setText(const std::wstring& text) override final;

	virtual std::wstring getText() const override final;

	virtual void setForeground() override final;

	virtual bool isForeground() const override final;

	virtual void setVisible(bool visible) override final;

	virtual bool isVisible() const override final;

	virtual void setEnable(bool enable) override final;

	virtual bool isEnable() const override final;

	virtual bool hasFocus() const override final;

	virtual void setFocus() override final;

	virtual bool hasCapture() const override final;

	virtual void setCapture() override final;

	virtual void releaseCapture() override final;

	virtual void startTimer(int interval, int id) override final;

	virtual void stopTimer(int id) override final;

	virtual void setRect(const Rect& rect) override final;

	virtual Rect getRect() const override final;

	virtual Rect getInnerRect() const override final;

	virtual Rect getNormalRect() const override final;

	virtual void setFont(const Font& font) override final;

	virtual Font getFont() const override final;

	virtual const IFontMetric* getFontMetric() const override final;

	virtual void setCursor(Cursor cursor) override final;

	virtual Point getMousePosition(bool relative) const override final;

	virtual Point screenToClient(const Point& pt) const override final;

	virtual Point clientToScreen(const Point& pt) const override final;

	virtual bool hitTest(const Point& pt) const override final;

	virtual void setChildRects(const IWidgetRect* childRects, uint32_t count) override final;

	virtual Size getMinimumSize() const override final;

	virtual Size getPreferedSize() const override final;

	virtual Size getMaximumSize() const override final;

	virtual void update(const Rect* rc, bool immediate) override final;

	virtual void* getInternalHandle() override final;

	virtual SystemWindow getSystemWindow() override final;

	// IFontMetric

	virtual void getAscentAndDescent(int32_t& outAscent, int32_t& outDescent) const override final T_FINAL;

	virtual int32_t getAdvance(wchar_t ch, wchar_t next) const override final T_FINAL;

	virtual int32_t getLineSpacing() const override final T_FINAL;

	virtual Size getExtent(const std::wstring& text) const override final T_FINAL;

	// INSWindowEventsCallback

	virtual void event_windowDidMove() override final;

	virtual void event_windowDidResize() override final;

	virtual bool event_windowShouldClose() override final;

	virtual void event_windowDidBecomeKey() override final;

	virtual void event_windowDidResignKey() override final;

	virtual void event_windowDidBecomeMain() override final;

	virtual void event_windowDidResignMain() override final;

private:
	EventSubject* m_owner;
	NSWindow* m_window;
	std::map< int, NSTimer* > m_timers;

	void callbackTimer(void* controlId);
};

	}
}

