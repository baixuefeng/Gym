#pragma once
#include <Windows.h>
//#include "boost/lockfree/queue.hpp"
#include <concurrent_queue.h>
#include "Thread/lockfree_queue.h"

BEGIN_SHARELIBTEST_NAMESPACE

struct TestParallelQueue
{
    void Test();

    void TestMsgQueue();

private:
    void BeforeTest();
    void AfterTest();

    static unsigned __stdcall PushThread(void *pVoid);
    static unsigned __stdcall PopThread(void *pVoid);

    static unsigned __stdcall MsgQueuePushThread(void *pVoid);
    static unsigned __stdcall MsgQueuePopThread(void *pVoid);

    HANDLE m_hEvent = nullptr;
    size_t m_testCount = 0;
    DWORD m_nThreadCount = 0;
    unsigned int m_threadid = 0;

    shr::lockfree_queue<MSG> m_queue;
    concurrency::concurrent_queue<MSG> m_queue2;
};

END_SHARELIBTEST_NAMESPACE
