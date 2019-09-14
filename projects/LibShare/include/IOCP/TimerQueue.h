#pragma once

#include "MacroDefBase.h"
#include <map>
#include <memory>
#include <mutex>
#include <windows.h>

SHARELIB_BEGIN_NAMESPACE

class TimerQueue
{
public:
    TimerQueue();
    ~TimerQueue();

    /** ��Ӷ�ʱ��
    @param[in] DueTime The amount of time in milliseconds relative to the current time
                       that must elapse before the timer is signaled for the first time.
    @param[in] Period The period of the timer, in milliseconds. If this parameter is zero,
                      the timer is signaled once. If this parameter is greater than zero, 
                      the timer is periodic. A periodic timer automatically reactivates 
                      each time the period elapses, until the timer is canceled.
    @param[in] TimerCallback ��ʱ���ص���ע�������̳߳ص��õġ�����ԭ�ͣ�call()
    @return timer���
    */
    template<typename TimerCallback>
    HANDLE AddTimer(DWORD DueTime, DWORD Period, TimerCallback && call)
    {
        if (!m_pTimerQueue)
        {
            return nullptr;
        }
        HANDLE hTimer = nullptr;
        using CallType = std::remove_reference<TimerCallback>::type;
        auto spCallback = std::make_shared<CallType>(std::forward<CallType>(call));
        if (::CreateTimerQueueTimer(
                &hTimer, 
                m_pTimerQueue, 
                TimerQueueTimerCallback<CallType>,
                spCallback.get(), 
                DueTime, 
                Period, 
                WT_EXECUTEDEFAULT) &&
            hTimer)
        {
            std::lock_guard<decltype(m_lock)> lock{ m_lock };
            m_timers[hTimer] = std::move(spCallback);
        }
        return hTimer;
    }

    /** �޸Ķ�ʱ��
    */
    bool ChangTimer(HANDLE hTimer, DWORD DueTime, DWORD Period);

    /** ɾ����ʱ������ͬ���ȴ��������еĶ�ʱ���ص�����
    */
    void RemoveTimer(HANDLE hTimer);

private:
    /** ��ʱ���ص�
    */
    template<class TimerCallback>
    static void CALLBACK TimerQueueTimerCallback(PVOID lpParameter, BOOLEAN /*TimerOrWaitFired*/)
    {
        TimerCallback * pCall = (TimerCallback*)lpParameter;
        if (pCall)
        {
            (*pCall)();
        }
    }

    /** ��鶨ʱ������Ƿ�Ϸ�
    */
    bool CheckTimerHandle(HANDLE hTimer);

private:
    // Timer Queue
    HANDLE m_pTimerQueue = nullptr;

    // timer lock
    std::mutex m_lock;

    // {��ʱ�� : ��ʱ���ص�}
    std::map<HANDLE, std::shared_ptr<void> > m_timers;
};

SHARELIB_END_NAMESPACE
