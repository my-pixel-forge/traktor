#include "Core/Misc/StringSplit.h"
#include "Flash/FlashSpriteInstance.h"
#include "Flash/Action/ActionContext.h"
#include "Flash/Action/ActionFunction.h"
#include "Flash/Action/ActionValueArray.h"

namespace traktor
{
	namespace flash
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.flash.ActionContext", ActionContext, Collectable)

ActionContext::ActionContext(const IActionVM* vm, const FlashMovie* movie)
:	m_vm(vm)
,	m_movie(movie)
{
}

void ActionContext::setGlobal(ActionObject* global)
{
	m_global = global;
}

void ActionContext::addFrameListener(ActionObject* frameListener)
{
	ActionValue memberValue;
	if (frameListener->getLocalMember("onFrame", memberValue))
	{
		Ref< ActionFunction > listenerFunction = memberValue.getObject< ActionFunction >();
		if (listenerFunction)
		{
			FrameListener fl = { frameListener, listenerFunction };
			m_frameListeners.push_back(fl);
		}
	}
}

void ActionContext::removeFrameListener(ActionObject* frameListener)
{
	for (std::vector< FrameListener >::iterator i = m_frameListeners.begin(); i != m_frameListeners.end(); ++i)
	{
		if (i->listenerTarget == frameListener)
		{
			m_frameListeners.erase(i);
			break;
		}
	}
}

void ActionContext::notifyFrameListeners(avm_number_t time)
{
	if (m_frameListeners.empty())
		return;

	ActionValueArray argv(m_pool, 1);
	argv[0] = ActionValue(time);

	std::vector< FrameListener > frameListeners = m_frameListeners;
	for (std::vector< FrameListener >::iterator i = frameListeners.begin(); i != frameListeners.end(); ++i)
		i->listenerFunction->call(i->listenerTarget, argv);
}

void ActionContext::pushMovieClip(FlashSpriteInstance* spriteInstance)
{
	m_movieClipStack.push_back(spriteInstance);
}

void ActionContext::popMovieClip()
{
	m_movieClipStack.pop_back();
}

FlashSpriteInstance* ActionContext::getMovieClip()
{
	return !m_movieClipStack.empty() ? m_movieClipStack.back() : 0;
}

ActionObject* ActionContext::lookupClass(const std::string& className)
{
	Ref< ActionObject > clazz = m_global;

	StringSplit< std::string > classNameSplit(className, ".");
	for (StringSplit< std::string >::const_iterator i = classNameSplit.begin(); i != classNameSplit.end(); ++i)
	{
		ActionValue clazzMember;
		if (!clazz->getLocalMember(*i, clazzMember) || !clazzMember.isObject())
			return 0;

		clazz = clazzMember.getObject();
	}

	if (!clazz)
		return 0;

	ActionValue prototypeMember;
	if (!clazz->getLocalMember("prototype", prototypeMember) || !prototypeMember.isObject())
		return 0;

	return prototypeMember.getObject();
}

void ActionContext::trace(const IVisitor& visitor) const
{
	for (RefArray< FlashSpriteInstance >::const_iterator i = m_movieClipStack.begin(); i != m_movieClipStack.end(); ++i)
		visitor(*i);

	visitor(m_global);

	for (std::vector< FrameListener >::const_iterator i = m_frameListeners.begin(); i != m_frameListeners.end(); ++i)
	{
		visitor(i->listenerTarget);
		visitor(i->listenerFunction);
	}
}

void ActionContext::dereference()
{
	m_movieClipStack.clear();
	m_global = 0;
	m_frameListeners.clear();
}

	}
}
