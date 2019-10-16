#include "IOCP/TimerQueue.h"
#include <cassert>

SHARELIB_BEGIN_NAMESPACE

TimerQueue::TimerQueue()
{
    m_pTimerQueue = ::CreateTimerQueue();
    assert(m_pTimerQueue);
}

TimerQueue::~TimerQueue()
{
    if (m_pTimerQueue) {
        for (auto &item : m_timers) {
            if (item.first) {
                ::DeleteTimerQueueTimer(m_pTimerQueue, item.first, INVALID_HANDLE_VALUE);
            }
        }
        m_timers.clear();
        ::DeleteTimerQueueEx(m_pTimerQueue, INVALID_HANDLE_VALUE);
        m_pTimerQueue = nullptr;
    }
}

bool TimerQueue::ChangTimer(HANDLE hTimer, DWORD DueTime, DWORD Period)
{
    if (m_pTimerQueue && CheckTimerHandle(hTimer)) {
        return !!::ChangeTimerQueueTimer(m_pTimerQueue, hTimer, DueTime, Period);
    }
    return false;
}

void TimerQueue::RemoveTimer(HANDLE hTimer)
{
    if (m_pTimerQueue && CheckTimerHandle(hTimer)) {
        ::DeleteTimerQueueTimer(m_pTimerQueue, hTimer, INVALID_HANDLE_VALUE);
        std::lock_guard<decltype(m_lock)> lock{m_lock};
        m_timers.erase(hTimer);
    }
}

bool TimerQueue::CheckTimerHandle(HANDLE hTimer)
{
#ifdef _DEBUG
    std::lock_guard<decltype(m_lock)> lock{m_lock};
    bool bOk = (m_timers.find(hTimer) != m_timers.end());
    assert(bOk);
    return bOk;
#else
    (void)(hTimer);
    return true;
#endif
}

SHARELIB_END_NAMESPACE
