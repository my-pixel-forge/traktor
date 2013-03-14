#include "Database/Local/Transaction.h"
#include "Database/Local/Action.h"
#include "Core/Thread/Mutex.h"
#include "Core/Log/Log.h"

#define T_USE_TRANSACTION_LOCK 0

namespace traktor
{
	namespace db
	{
		namespace
		{

const int c_transactionTimeout = 100;

		}

T_IMPLEMENT_RTTI_CLASS(L"traktor.db.Transaction", Transaction, Object)

Transaction::Transaction()
#if defined(_DEBUG)
:	m_locked(false)
#endif
{
}

Transaction::~Transaction()
{
#if defined(_DEBUG)
	T_ASSERT_M (!m_locked, L"Transaction still locked");
#endif
}

bool Transaction::create(const Guid& transactionGuid)
{
	T_ASSERT_M (!m_locked, L"Transaction already created");

#if T_USE_TRANSACTION_LOCK
	m_lock = new Mutex(transactionGuid);
	if (!m_lock->acquire(c_transactionTimeout))
	{
		T_DEBUG(L"Unable to create transaction \"" << m_transactionGuid.format() << L"\"; already exclusively locked");
		m_lock->release();
		m_lock = 0;
		return false;
	}
#endif

#if defined(_DEBUG)
	m_locked = true;
#endif

	m_transactionGuid = transactionGuid;
	T_DEBUG(L"Transaction \"" << m_transactionGuid.format() << L"\" created successfully");

	return true;
}

void Transaction::destroy()
{
	T_ASSERT_M (m_locked, L"Transaction already destroyed");

#if T_USE_TRANSACTION_LOCK
	if (m_lock)
	{
		m_lock->release();
		m_lock = 0;
	}
#endif

#if defined(_DEBUG)
	m_locked = false;
#endif

	T_DEBUG(L"Transaction \"" << m_transactionGuid.format() << L"\" destroyed");
}

void Transaction::add(Action* action)
{
	m_actions.push_back(action);
}

bool Transaction::commit(Context* context)
{
	T_ASSERT_M (m_locked, L"Transaction not created");

	bool result = true;

	// Execute actions.
	for (int32_t i = 0; i < int32_t(m_actions.size()); ++i)
	{
		if (!m_actions[i]->execute(context))
		{
			log::error << L"Transaction action " << type_name(m_actions[i]) << L" failed; performing rollback" << Endl;
			for (int32_t j = i; j >= 0; --j)
			{
				if (!m_actions[j]->undo(context))
					log::error << L"Unable to undo transaction step " << j << L"; data integrity might be corrupt" << Endl;
			}
			result = false;
			break;
		}
	}

	// Cleanup action temporary data.
	for (int32_t i = 0; i < int32_t(m_actions.size()); ++i)
		m_actions[i]->clean(context);

	if (result)
		T_DEBUG(L"Transaction \"" << m_transactionGuid.format() << L"\" commited successfully");
	else
		T_DEBUG(L"Transaction \"" << m_transactionGuid.format() << L"\" failed to commit");

	m_actions.clear();
	return result;
}

	}
}
