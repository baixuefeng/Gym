#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <Windows.h>
#include <atlbase.h>
#include <atlmem.h>
#include "IOCP/IOCPThreadPool.h"
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

class SimpleIOCPPipeCenter
{
    SHARELIB_DISABLE_COPY_CLASS(SimpleIOCPPipeCenter);

    //自定义的OverLapped结构
    struct TPipeOverLapped;

public:
    SimpleIOCPPipeCenter();
    virtual ~SimpleIOCPPipeCenter();

    /** 创建并获取线程池，外部可籍此复用线程池.可以不调用，CreatePipe或ConnectToServer时会使用默认值调用，
    如果不希望使用默认参数，先自行调用并传入自己的值。
    @param [in] dwNumOfRun 同时激活的线程数，如果为0，取CPU个数值；如果比dwNumOfMax大，取dwNumOfMax
    @param [in] dwNumOfMax 线程池中最大等待线程数，如果为0，取CPU个数值 * 2
    */
    IOCPThreadPool &GetThreadPool(DWORD dwNumOfRun = 0, DWORD dwNumOfMax = 0);

    /** 创建命名管道, 注意, 返回的管道句柄不要使用CloseHandle关闭
    @param[in,out] name 管道名。如果为空，则自动生成一个名字，并返回；如果非空，则使用它来创建Pipe
    @return 成功返回管道句柄, 否则返回 NULL
    */
    HANDLE CreatePipe(std::wstring &name);

    /** 连接服务器, 注意, 返回的管道句柄不要使用CloseHandle关闭
    @param[in] name 管道名
    @return 成功返回管道句柄, 否则返回 NULL
    */
    HANDLE ConnectToServer(const std::wstring &name);

    /** 获取命名管道的名字 
    @param[in] hPipe 管道句柄
    @return 名字
    */
    std::wstring GetPipeName(HANDLE hPipe);

    /** 异步等待客户端连接上来,操作结果通过OnClientConnected回调
    @param[in] hPipe CreatePipe返回的管道句柄
    @param[in] pUserData 用户自定义数据
    @return 操作是否成功
    */
    bool AsyncWaitForClientConnect(HANDLE hPipe, void *pUserData);

    /** 发送缓存区，把要发送的数据添加进缓存区，而后用AsyncSend发送。
    */
    class SendBuffer
    {
    public:
        /** 移动构造
        */
        SendBuffer(SendBuffer &&other);

        /** 析构函数
        */
        ~SendBuffer();

        /** 向缓存区添加数据，添加的数据总长度不能超过 GetSendBuffer 指定的长度
        @param[in] pData 数据指针
        @param[in] nDataLength 数据长度
        @return 添加是否成功
        */
        bool AddData(const void *pData, uint32_t nDataLength);

    private:
        friend class SimpleIOCPPipeCenter;

        /** 默认构造
        */
        SendBuffer();

        /** 内部缓存数据 
        */
        TPipeOverLapped *m_pOvlp;
    };

    /** 获取一个发送缓存，用于向其中填写要发送的数据，而后交给AsyncSend发送出去
    @param[in] nDataLength 缓存区最大可写入的数据长度
    @return 发送缓存
    */
    SendBuffer GetSendBuffer(uint32_t nDataLength);

    /** 异步发送,操作结果通过 OnSend 回调.
    @param[in] hPipe 从 CreatePipe 返回的管道句柄
    @param[in] buffer 从 GetSendBuffer 返回的写缓存。
    @param[in] pUserData 用户自定义数据
    @return 操作是否成功
    */
    bool AsyncSend(HANDLE hPipe, SendBuffer &&buffer, void *pUserData);

    /** 异步接收,操作结果通过 OnReceive 回调
    @param[in] hPipe 从 CreatePipe 返回的管道句柄
    @param[in] nHeaderSize 包头大小, 必须设置为大于0, 表示一次接收时最小接收长度, 后序完整接收的数据长度
               也需要从包头中解析出来.
    @param[in] pUserData 用户自定义数据
    @return 操作是否成功
    */
    bool AsyncReceive(HANDLE hPipe, uint32_t nHeaderSize, void *pUserData);

    /** 关闭管道
    */
    void ClosePipe(HANDLE hPipe);

    /** 关闭所有,非多线程安全.注意不能在回调中调用该函数,会死锁.
        通常需要在派生类析构之前调用该函数,否则如果在派生类半析构的时候,又产生了IO回调,就会出现
        "pure virtual function call"的崩溃.
    */
    void CloseAllPipes();

protected:
    //----回调函数，注意，这些是多线程回调，与投递异步请求的线程不在同一线程，注意多线程安全问题-----------------

    /** 有客户端连接上来回调
    @param[in] bSuccess 连接是否成功
    @param[in] hPipe AsyncWaitForClientConnect中传入的参数hPipe
    @param[in] pUserData AsyncWaitForClientConnect中传入的参数pUserData
    */
    virtual void OnClientConnected(bool bSuccess, HANDLE hPipe, void *pUserData) = 0;

    /** 发送回调
    @param[in] bSuccess 发送是否成功
    @param[in] hPipe AsyncSend中传入的参数hPipe
    @param[in] pData 用户指定要发送的数据
    @param[in] nDataLength 用户指定要发送的数据长度
    @param[in] nNumberOfBytesTransferred 实际成功发送的数据长度
    @param[in] pUserData AsyncSend中传入的参数pUserData
    */
    virtual void OnSend(bool bSuccess,
                        HANDLE hPipe,
                        void *pData,
                        size_t nDataLength,
                        size_t nNumberOfBytesTransferred,
                        void *pUserData) = 0;

    /** 从接收到的数据包头中解析出数据总长(包含包头本身的大小)
    @param[in] hPipe AsyncReceive中传入的参数hPipe
    @param[in] pHeader 数据包头
    @param[in] nHeaderSize 包头大小, 等于AsyncReceive中设定的包头大小
    @param[in] pUserData AsyncReceive中传入的参数pUserData
    @return 数据总长(包含包头本身的大小)
    */
    virtual uint32_t GetDataLengthFromHeader(HANDLE hPipe,
                                             const void *pHeader,
                                             uint32_t nHeaderSize,
                                             void *pUserData) = 0;

    /** 接收回调, 如果成功, 会是一条完整的数据, 即数据总长等于从数据包头中解析出来的数据总长
    @param[in] bSuccess 接收是否成功
    @param[in] hPipe AsyncReceive中传入的参数hPipe
    @param[in] pData 接收到的数据
    @param[in] nDataLength 实际接收到的数据长度
    @param[in] pUserData AsyncReceive中传入的参数pUserData
    */
    virtual void OnReceive(bool bSuccess,
                           HANDLE hPipe,
                           void *pData,
                           size_t nDataLength,
                           void *pUserData) = 0;

private:
    /** 检查管道句柄是否合法
    @param[in] hPipe 管道句柄
    */
    bool CheckPipe(HANDLE hPipe);

    /** 分配一个OVERLAPPED结构
    @param[in] nDataLength 最大数据长度
    */
    TPipeOverLapped *AllocOverLapped(uint32_t nDataLength);

    /** 重新分配TPipeOverLapped结构的大小
    @param[in,out] pOvlp TPipeOverLapped结构指针
    @param[in] nDataLength 最大数据长度
    @return 是否成功
    */
    bool ReallocOverLapped(TPipeOverLapped *&pOvlp, uint32_t nNewDataLength);

    /** 线程池IO完成回调
    @param[in] dwErrno 错误代码
    @param[in] dwNumberOfBytesTransferred 成功传输的数据长度
    @param[in] pOverlapped 回调参数
    */
    static void OnIOCompleted(DWORD dwErrno,
                              DWORD dwNumberOfBytesTransferred,
                              LPOVERLAPPED pOverlapped);

    /** 线程池IO完成回调
    @param[in] dwErrno 错误代码
    @param[in] dwNumberOfBytesTransferred 成功传输的数据长度
    @param[in] pOvlp 回调参数
    */
    void OnIOCompletedImpl(DWORD dwErrno, DWORD dwNumberOfBytesTransferred, TPipeOverLapped *pOvlp);

private:
    //线程池
    IOCPThreadPool m_threadPool;

    //线程锁
    ATL::CComAutoCriticalSection m_lock;

    //pipe pool
    std::unordered_map<HANDLE, std::wstring> m_pipePool;

    //私有堆
    ATL::CWin32Heap m_privateHeap;
};

SHARELIB_END_NAMESPACE
