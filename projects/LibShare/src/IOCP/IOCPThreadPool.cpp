#include "targetver.h"
#include "IOCP/IOCPThreadPool.h"
#include <chrono>
#include <cassert>
#include <atomic>
#include <mutex>
#include <exception>
#include <condition_variable>
#include <memory>
#include <process.h>

SHARELIB_BEGIN_NAMESPACE

//工作线程栈大小
static const unsigned int STACK_SIZE = 1024 * 1024 * 3;

static inline bool IsIOCPQuitKey(ULONG_PTR lpCompletionKey)
{
    return lpCompletionKey == 0;
}

struct IOCPThreadPool::IOCPContex
{
    IOCPContex()
        : m_dwMaxWaitNum(0)
        , m_dwCurrNum(0)
        , m_dwBusyNum(0)
        , m_hIOCP(nullptr)
        , m_dwTimerOut(10000)
    {
    }

    ~IOCPContex()
    {
        if (nullptr != m_hIOCP)
        {
            ::CloseHandle(m_hIOCP);
            m_hIOCP = nullptr;
        }
    }

    /** 初始化IOCP
    @param[in] dwNumOfRun 同时运行的线程数
    @param[in] dwNumOfMax 最大线程数
    @return 是否成功
    */
    bool InitContext(DWORD dwNumOfRun, DWORD dwNumOfMax)
    {
        if (dwNumOfRun > dwNumOfMax && dwNumOfMax != 0)
        {
            dwNumOfRun = dwNumOfMax;
        }
        if (dwNumOfMax != 0)
        {
            m_dwMaxWaitNum = dwNumOfMax;
        }
        else
        {
            SYSTEM_INFO sysInfo;
            ::GetSystemInfo(&sysInfo);
            m_dwMaxWaitNum = sysInfo.dwNumberOfProcessors * 2;
        }

        m_hIOCP = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, dwNumOfRun);
        if (nullptr == m_hIOCP)
        {
            return false;
        }
        return true;
    }

    /** 往线程池中添加线程
    */
    bool AddThread()
    {
        HANDLE hThread = (HANDLE)::_beginthreadex(0, STACK_SIZE, IOCPWorkThread, this, CREATE_SUSPENDED, 0);
        if (nullptr != hThread)
        {
            //由启动者增加计数，避免启动者退出、但新线程还没有执行增加计数的代码导致计数错误
            ++m_dwCurrNum;
            ::ResumeThread(hThread);
            ::CloseHandle(hThread);
            hThread = nullptr;
            return true;
        }
        return false;
    }

    /** 往线程池中投递退出信号，使线程池退出
    */
    void PostIOCPQuitKey()
    {
        while (!::PostQueuedCompletionStatus(m_hIOCP, 0, 0, nullptr))
        {
            ;
        }
    }

    //最大等待线程数
    std::atomic<DWORD> m_dwMaxWaitNum;

    //当前线程数
    std::atomic<DWORD> m_dwCurrNum;

    //正在工作的线程数
    std::atomic<DWORD> m_dwBusyNum;

    //线程锁
	std::mutex m_threadLock;

    //当前时间
    std::chrono::time_point<std::chrono::system_clock> m_timeCur;

    //Iocp句柄
    HANDLE m_hIOCP;

    //超时时间
    DWORD m_dwTimerOut;

    //退出事件
    std::condition_variable m_quitEvent;
};

IOCPThreadPool::IOCPThreadPool()
    : m_pIOCPContex(nullptr)
{
}

IOCPThreadPool::~IOCPThreadPool()
{
    if (m_pIOCPContex)
    {
        CloseIOCPThreadPool();
    }
}

bool IOCPThreadPool::CreateIOCPThreadPool(DWORD dwNumOfRun /*= 0*/, DWORD dwNumOfMax /*= 0*/)
{
    if (m_pIOCPContex)
    {
        return true;
    }

    std::unique_ptr<IOCPContex> spIocpContex;
    try
    {
        spIocpContex.reset(new IOCPContex);
    }
    catch (...)
    {
        return false;
    }
    if (!spIocpContex->InitContext(dwNumOfRun, dwNumOfMax) ||
        !spIocpContex->AddThread())
    {
        return false;
    }
    m_pIOCPContex = spIocpContex.release();
    return true;
}

void IOCPThreadPool::CloseIOCPThreadPool()
{
    if (m_pIOCPContex == nullptr)
    {
        return;
    }

    /* 退出方法：推消息使等待线程取消等待，进而检测到退出标志退出
       这里只Post一个退出消息，每个线程收到退出消息后、退出之前，再Post一个退出消息，这样可以确保一定能退出干净。
       用线程池最大线程数的话是不可靠的，可能出现先人为扩大最大线程数限制，当实际线程数增加后再减少最大线程数限制，
       这时会出现实际线程数大于最大线程数限制的情况。
    */
    m_pIOCPContex->PostIOCPQuitKey();

    {
        std::unique_lock<decltype(m_pIOCPContex->m_threadLock)> lock(m_pIOCPContex->m_threadLock);
        if (m_pIOCPContex->m_dwCurrNum > 0)
        {
            m_pIOCPContex->m_quitEvent.wait(lock);
        }
    }
    delete m_pIOCPContex;
    m_pIOCPContex = nullptr;
}

IOCPThreadPool::operator bool() const
{
    return !this->operator! ();
}

bool IOCPThreadPool::operator!() const
{
    return m_pIOCPContex == nullptr;
}

void IOCPThreadPool::SetExpiryTime(DWORD dwSeconds)
{
    assert(m_pIOCPContex);
    if (m_pIOCPContex == nullptr)
    {
        return;
    }
    if (dwSeconds > 0)
    {
        m_pIOCPContex->m_dwTimerOut = dwSeconds * 1000;
    }
    else
    {
        m_pIOCPContex->m_dwTimerOut = INFINITE;
    }
}

bool IOCPThreadPool::BindDeviceToIOCPThreadPool(HANDLE hFileHandle, TPFOnIOCompleted pfOnIOCompleted)
{
    assert(m_pIOCPContex);
    if (m_pIOCPContex == nullptr)
    {
        return false;
    }
    if ((hFileHandle == nullptr) ||
        (hFileHandle == INVALID_HANDLE_VALUE))
    {
        return false;
    }
    if (IsIOCPQuitKey((ULONG_PTR)pfOnIOCompleted))
    {
        return false;
    }

    HANDLE h = ::CreateIoCompletionPort(hFileHandle, m_pIOCPContex->m_hIOCP, (ULONG_PTR)pfOnIOCompleted, 0);
    if (h != m_pIOCPContex->m_hIOCP)
    {
        assert(!"设备句柄绑定到IOCP失败");
        return false;
    }
    return true;
}

bool IOCPThreadPool::AsyncCall(TPFIOCPAsyncCallBack pfOnIOCompleted, DWORD dwNumberOfBytesTransferred, void *pVoid)
{
    assert(m_pIOCPContex);
    if (m_pIOCPContex == nullptr)
    {
        return false;
    }
    if (IsIOCPQuitKey((ULONG_PTR)pfOnIOCompleted))
    {
        return false;
    }

    return !!::PostQueuedCompletionStatus(m_pIOCPContex->m_hIOCP, dwNumberOfBytesTransferred, (ULONG_PTR)pfOnIOCompleted, (LPOVERLAPPED)pVoid);
}

unsigned int __stdcall IOCPThreadPool::IOCPWorkThread(void * pVoid)
{
    IOCPContex *pIOCPContext = (IOCPContex*)(pVoid);

    DWORD dwNumberOfBytesTransferred = 0;
    ULONG_PTR nCompletionKey = 0;
    LPOVERLAPPED pOverlapped = nullptr;
    BOOL bOK = FALSE;
    DWORD dwErrno = ERROR_SUCCESS;

    for (;;)
    {
        dwNumberOfBytesTransferred = 0;
        nCompletionKey = 0;
        pOverlapped = nullptr;
        bOK = ::GetQueuedCompletionStatus(
            pIOCPContext->m_hIOCP,
            &dwNumberOfBytesTransferred,
            &nCompletionKey,
            &pOverlapped,
            pIOCPContext->m_dwTimerOut);

        dwErrno = ::GetLastError();
        if (bOK || (pOverlapped != nullptr))
        {
            if (bOK && IsIOCPQuitKey(nCompletionKey))
            {
                pIOCPContext->PostIOCPQuitKey();
                break;
            }

            /* 满足以下条件时增加线程池中线程的个数:
            1.线程池中线程总数小于等于正在工作中的线程数；2.线程池中线程总数小于最大值
            */
            if ((pIOCPContext->m_dwCurrNum <= ++pIOCPContext->m_dwBusyNum) &&
                (pIOCPContext->m_dwCurrNum < pIOCPContext->m_dwMaxWaitNum))
            {
                pIOCPContext->AddThread();
            }

            //调用回调函数
            try
            {
                if (bOK)
                {
                    dwErrno = ERROR_SUCCESS;
                }
                ((TPFOnIOCompleted)nCompletionKey)(dwErrno, dwNumberOfBytesTransferred, pOverlapped);
            }
#ifdef _DEBUG
            catch (const std::exception& errMsg)
            {
                ::MessageBoxA(nullptr, errMsg.what(), nullptr, MB_OK);
                assert(!"TPFOnIOCompleted抛出了异常！");
            }
#endif
            catch (...)
            {
                assert(!"TPFOnIOCompleted抛出了未知异常！");
            }

            --pIOCPContext->m_dwBusyNum;
        }
        else if (!bOK && (WAIT_TIMEOUT == dwErrno))
        {
            std::unique_lock<decltype(pIOCPContext->m_threadLock)>
                tryLock{ pIOCPContext->m_threadLock, std::try_to_lock_t() };
            if (tryLock)
            {
                /* 满足这两个条件的时候, 每隔 m_dwTimerOut 时间减少一个线程。
                1.当前线程池中总线程数大于正在工作线程数的2倍；2.当前线程池中总线程数大于2。
                */
                if ((pIOCPContext->m_dwBusyNum * 2 < pIOCPContext->m_dwCurrNum) && 
                    (pIOCPContext->m_dwCurrNum > 2))
                {
                    using namespace std::chrono;
                    if ((pIOCPContext->m_timeCur.time_since_epoch().count() == 0) ||
                        (duration_cast<milliseconds>(system_clock::now() - pIOCPContext->m_timeCur).count() >= pIOCPContext->m_dwTimerOut - 100))
                    {
                        pIOCPContext->m_timeCur = system_clock::now();
                        break;
                    }
                }
            }
        }
        else
        {
            assert(!"GetQueuedCompletionStatus函数调用错误");
            break;
        }
    }

    std::unique_lock<decltype(pIOCPContext->m_threadLock)> lock(pIOCPContext->m_threadLock);
    if (--pIOCPContext->m_dwCurrNum == 0)
    {
        pIOCPContext->m_quitEvent.notify_all();
    }
    return 0;
}

SHARELIB_END_NAMESPACE
