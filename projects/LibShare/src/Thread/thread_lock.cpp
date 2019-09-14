#include <chrono>
#include "Thread/thread_lock.h"

SHARELIB_BEGIN_NAMESPACE

/** 条件变量等待的辅助函数
@param[in] lock 线程锁
@param[in] conVar 条件变量，与lock配合一起使用
@param[in] pred 判断条件，调用原型： bool pred();
@param[in] nMilliseconds 0表示不等待，<0表示永久等待，>0表示最多等待指定的时间(毫秒)
@return 返回true表示成功等待到pred为true的事件
*/
template<class TLock, class TConditionVariable, class TPred>
static bool WaitCondition(TLock & lock, TConditionVariable & conVar, TPred && pred, int64_t nMilliseconds)
{
    std::unique_lock<TLock> autoLock(lock);
    if (nMilliseconds < 0)
    {
        conVar.wait(autoLock, std::forward<TPred>(pred));
        return true;
    }
    else if (nMilliseconds == 0)
    {
        return pred();
    }
    else
    {
        return conVar.wait_for(autoLock, std::chrono::milliseconds(nMilliseconds), std::forward<TPred>(pred));
    }
}

//------------------------------------------------------

EventLock::EventLock(bool bManualReset, bool bInitialState)
    : m_bManualReset(bManualReset)
    , m_bActive(bInitialState)
{
}

void EventLock::SetEvent()
{
    std::unique_lock<decltype(m_lock)> lock{ m_lock };
    if (!m_bActive)
    {
        m_bActive = true;
        if (m_bManualReset)
        {
            m_condition.notify_all();
        }
        else
        {
            m_condition.notify_one();
        }
    }
}

void EventLock::ResetEvent()
{
    std::unique_lock<decltype(m_lock)> lock{ m_lock };
    m_bActive = false;
}

bool EventLock::Wait(int64_t nMilliseconds /*= -1*/)
{
    auto && pred = [this]()->bool
    {
        if (m_bActive && !m_bManualReset)
        {
            m_bActive = false;
            return true;
        }
        else
        {
            return m_bActive;
        }
    };
    return WaitCondition(m_lock, m_condition, pred, nMilliseconds);
}

//-----------------------------------------------------------------------

Semaphore::Semaphore(size_t nInitCount)
    : m_count(nInitCount)
{
}

bool Semaphore::Acquire(size_t nCount, int64_t nMilliseconds/* = -1*/)
{
	if (nCount == 0)
	{
		return true;
	}
    auto && pred = [nCount, this]()->bool
    {
        if (m_count >= nCount)
        {
            m_count -= nCount;
            return true;
        }
        return false;
    };
    return WaitCondition(m_lock, m_condition, pred, nMilliseconds);
}

size_t Semaphore::Release(size_t nCount, bool bNotifyAll /*= true*/)
{
    std::unique_lock<decltype(m_lock)> autuLock{ m_lock };
    if (nCount > 0)
    {
        m_count += nCount;
        if (bNotifyAll && (m_count > 1))
        {
            m_condition.notify_all();
        }
        else
        {
            m_condition.notify_one();
        }
    }
    return m_count;
}

//----------------------------------------------------------------------

SHARELIB_END_NAMESPACE
