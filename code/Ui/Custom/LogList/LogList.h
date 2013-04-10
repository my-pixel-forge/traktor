#ifndef traktor_ui_custom_LogList_H
#define traktor_ui_custom_LogList_H

#include <list>
#include "Core/Thread/Semaphore.h"
#include "Ui/Widget.h"

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
class ScrollBar;

		namespace custom
		{

/*! \brief Log list control.
 * \ingroup UIC
 */
class T_DLLCLASS LogList : public Widget
{
	T_RTTI_CLASS;

public:
	enum LogLevel
	{
		LvInfo		= 1 << 0,
		LvWarning	= 1 << 1,
		LvError		= 1 << 2,
		LvDebug		= 1 << 3
	};

	LogList();

	bool create(Widget* parent, int style);

	void add(LogLevel level, const std::wstring& text);

	void removeAll();

	void setFilter(uint32_t filter);

	uint32_t getFilter() const;

	bool copyLog(uint32_t filter = ~0UL);

	virtual Size getPreferedSize() const;

private:
	struct Entry
	{
		uint32_t threadId;
		LogLevel logLevel;
		std::wstring logText;
	};

	typedef std::list< Entry > log_list_t;

	Ref< ScrollBar > m_scrollBar;
	Ref< Bitmap > m_icons;
	log_list_t m_pending;
	Semaphore m_pendingLock;
	log_list_t m_logFull;
	log_list_t m_logFiltered;
	std::map< uint32_t, uint32_t > m_threadIndices;
	int32_t m_itemHeight;
	uint32_t m_filter;
	uint32_t m_nextThreadIndex;

	void updateScrollBar();

	void eventPaint(Event* event);

	void eventSize(Event* event);

	void eventMouseWheel(Event* event);

	void eventScroll(Event* event);

	void eventTimer(Event* event);
};

		}
	}
}

#endif	// traktor_ui_custom_LogList_H
