#pragma once
#include <future>
#include <windows.h>
#include "MacroDefBase.h"
//线程池回调函数不允许使用ExitThread
#pragma deprecated(ExitThread)

/*!
 * \file IocpThreadPool.h
 * \brief IOCP线程池封装.
 基于IOCP封装，win32 API GetQueuedCompletionStatus中占用第三个参数lpCompletionKey，作为函数指针使用。
 */

SHARELIB_BEGIN_NAMESPACE

//---------------------------------------

//异步IO完成的回调函数，注意，会有多个线程同时调用此函数
typedef void (*TPFOnIOCompleted)(DWORD dwErrno, //ERROR_SUCCESS表示成功
                                 DWORD dwNumberOfBytesTransferred,
                                 LPOVERLAPPED pOverlapped);

//异步任务回调
typedef void (*TPFIOCPAsyncCallBack)(DWORD /*dwErrno*/, //该参数不使用
                                     DWORD dwNumberOfBytesTransferred,
                                     void *pVoid);

class IOCPThreadPool
{
    SHARELIB_DISABLE_COPY_CLASS(IOCPThreadPool);

public:
    IOCPThreadPool();
    ~IOCPThreadPool();

    //----创建和销毁不是多线程安全的，不要多个线程针对一个对象同时调用--------------------------------

    /** 创建IOCP、线程池，并且二者关联在一起。
    注意防止因线程池数量导致的死锁：一个异步函数执行了一个等待操作，需要后一个异步函数使其解除等待，
    但由于已达到最大数量的限制，后一个函数不会执行，前一个函数一直等待并占用线程池，导致死锁。
    @param [in] dwNumOfRun 同时激活的线程数，如果为0，取CPU个数值；如果比dwNumOfMax大，取dwNumOfMax
    @param [in] dwNumOfMax 线程池中最大等待线程数，如果为0，取CPU个数值 * 2
    */
    bool CreateIOCPThreadPool(DWORD dwNumOfRun = 0, DWORD dwNumOfMax = 0);

    /** 结束线程池的运行并关闭IOCP，会同步等待所有线程退出后函数才返回。注意不能在IOCP回调中调用该函数,会死锁.
    */
    void CloseIOCPThreadPool();

    //----下面的接口是多线程安全的--------------------------------------------------------------

    //线程池是否创建成功
    explicit operator bool() const;
    bool operator!() const;

    /** 设置线程池中的线程空闲退出的时间，每次退出一个线程，
    @param[in] dwSeconds 空闲退出时间，0表示无穷.默认值为10.
    */
    void SetExpiryTime(DWORD dwSeconds);

    /** 把设备句柄与IOCP关联在一起。
    @param[in] hFileHandle 设备句柄
    @param[in] pfOnIOCompleted IO完成时的回调函数，不可为空
    @return 操作是否成功
    */
    bool BindDeviceToIOCPThreadPool(HANDLE hFileHandle, TPFOnIOCompleted pfOnIOCompleted);

    /** 向IOCP发送消息，使其异步执行函数pfOnIOCompleted，函数的参数则由 dwNumberOfBytesTransferred，
    pVoid指定，与BindDeviceToIOCPThreadPool类似，不可为空。实际封装的是封装PostQueuedCompletionStatus，
    这种情况下，TPFIOCPAsyncCallBack回调中的dwErrno将总是ERROR_SUCCESS，而原本应该是LPOVERLAPPED的指针，
    由于IOCP内部并不会使用它，所以可以传任意值，因此最后一个参数由LPOVERLAPPED变为void*
    @param[in] pfOnIOCompleted 异步回调函数
    @param[in] dwNumberOfBytesTransferred 自定义回调参数
    @param[in] pVoid 自定义回调参数
    @return 操作是否成功
    */
    bool AsyncCall(TPFIOCPAsyncCallBack pfOnIOCompleted,
                   DWORD dwNumberOfBytesTransferred,
                   void *pVoid);

    /** IOCP异步调用，在AsyncCall的基础上再次封装，使其形成类型安全的、支持
    Lambda表达式调用的一个接口
    @param[in] callObj 调用对象, lambda表达式, 调用原型: 任意返回值 Func();
    @return std::future
    */
    template<class _Callable>
    std::future<std::result_of_t<typename std::decay_t<_Callable>()>> AsyncCall(_Callable &&callObj)
    {
        using resultType = std::result_of_t<typename std::decay_t<_Callable>()>;
        using taskType = std::packaged_task<resultType()>;

        if (m_pIOCPContex == nullptr) {
            return std::future<resultType>();
        }
        taskType *pTask = new taskType(std::forward<_Callable>(callObj));
        if (!AsyncCall(AsyncCallHelper<taskType>, 0, pTask)) {
            delete pTask;
            return std::future<resultType>();
        } else {
            return pTask->get_future();
        }
    }

private:
    /** AsyncCall 的辅助函数
    */
    template<class taskType>
    static void AsyncCallHelper(DWORD /*dwErrno*/,
                                DWORD /*dwNumberOfBytesTransferred*/,
                                void *pVoid)
    {
        taskType *pTask = (taskType *)pVoid;
        (*pTask)();
        delete pTask;
    }

    /** 线程池工作线程
    */
    static unsigned int __stdcall IOCPWorkThread(void *pVoid);

private:
    //Iocp内部实现
    struct IOCPContex;
    IOCPContex *m_pIOCPContex;
};

SHARELIB_END_NAMESPACE
