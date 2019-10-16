#include "stdafx.h"
#include "TestUnit/TestParallelQueue.h"
#include <iostream>
#include <vector>
#include <boost/timer/timer.hpp>
#include <process.h>
#include "Log/TempLog.h"

BEGIN_SHARELIBTEST_NAMESPACE

void TestParallelQueue::BeforeTest()
{
    m_threadid = 0;
    tcout << "输入测试数量(万):";
    std::cin >> m_testCount;
    m_testCount *= 10000;

    tcout << "输入测试线程对数:";
    std::cin >> m_nThreadCount;
    if (m_testCount == 0 || m_nThreadCount == 0) {
        return;
    }

    m_hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
}

void TestParallelQueue::AfterTest()
{
    ::CloseHandle(m_hEvent);
    m_hEvent = NULL;
    m_testCount = 0;
    m_nThreadCount = 0;
}

void TestParallelQueue::Test()
{
    BeforeTest();

    std::vector<HANDLE> threads;
    threads.assign(m_nThreadCount * 2, NULL);
    for (DWORD i = 0; i < m_nThreadCount; ++i) {
        threads[i * 2] = (HANDLE)::_beginthreadex(0, 0, PushThread, this, 0, 0);
        threads[i * 2 + 1] = (HANDLE)::_beginthreadex(0, 0, PopThread, this, 0, 0);
    }
    ::Sleep(100);
    boost::timer::auto_cpu_timer timer;
    ::SetEvent(m_hEvent);
    ::WaitForMultipleObjects(m_nThreadCount * 2, &threads[0], TRUE, INFINITE);
    timer.stop();
    timer.report();
    for (auto h : threads) {
        ::CloseHandle(h);
    }

    AfterTest();
}

void TestParallelQueue::TestMsgQueue()
{
    BeforeTest();

    HANDLE threads[2] = {0};
    threads[0] = (HANDLE)::_beginthreadex(0, 0, MsgQueuePushThread, this, 0, 0);
    threads[1] = (HANDLE)::_beginthreadex(0, 0, MsgQueuePopThread, this, 0, &m_threadid);

    ::Sleep(100);
    boost::timer::auto_cpu_timer timer;
    ::SetEvent(m_hEvent);
    ::WaitForMultipleObjects(m_nThreadCount * 2, &threads[0], TRUE, INFINITE);
    timer.stop();
    timer.report();
    for (auto h : threads) {
        ::CloseHandle(h);
    }

    AfterTest();
}

unsigned __stdcall TestParallelQueue::PushThread(void *pVoid)
{
    TestParallelQueue *pThis = (TestParallelQueue *)pVoid;
    ::WaitForSingleObject(pThis->m_hEvent, INFINITE);

    MSG msg{};
    for (size_t i = 0; i < pThis->m_testCount; ++i) {
        pThis->m_queue.push(msg);
    }
    return 0;
}

unsigned __stdcall TestParallelQueue::PopThread(void *pVoid)
{
    TestParallelQueue *pThis = (TestParallelQueue *)pVoid;
    ::WaitForSingleObject(pThis->m_hEvent, INFINITE);
    MSG msg{};
    for (size_t i = 0; i < pThis->m_testCount; ++i) {
        while (!pThis->m_queue.try_pop(msg)) {
        }
    }
    return 0;
}

unsigned __stdcall TestParallelQueue::MsgQueuePushThread(void *pVoid)
{
    TestParallelQueue *pThis = (TestParallelQueue *)pVoid;
    ::WaitForSingleObject(pThis->m_hEvent, INFINITE);
    for (size_t i = 0; i < pThis->m_testCount; ++i) {
        while (!::PostThreadMessage(pThis->m_threadid, WM_USER + 1, 1, 2)) {
        }
    }
    return 0;
}

unsigned __stdcall TestParallelQueue::MsgQueuePopThread(void *pVoid)
{
    TestParallelQueue *pThis = (TestParallelQueue *)pVoid;
    ::WaitForSingleObject(pThis->m_hEvent, INFINITE);
    MSG msg{};
    for (size_t i = 0; i < pThis->m_testCount; ++i) {
        while (!::PeekMessage(&msg, (HWND)-1, 0, 0, PM_REMOVE)) {
        }
    }
    return 0;
}

END_SHARELIBTEST_NAMESPACE
