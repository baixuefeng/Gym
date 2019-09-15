#include "targetver.h"
#include "IOCP/SimpleIOCPPipeCenter.h"
#include <cassert>
#include <cstring>
#include <type_traits>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

SHARELIB_BEGIN_NAMESPACE

//内存分配粒度
static size_t g_allocationGranularity = 0;

//操作类型
enum TOperatationType
{
    OP_INVALID = -1, //无效值
    OP_CONNECT,
    OP_SEND,
    OP_RECEIVE
};

struct SimpleIOCPPipeCenter::TPipeOverLapped : public OVERLAPPED
{
    TPipeOverLapped()
    {
        std::memset(this, 0, sizeof(*this));
        m_opType = TOperatationType::OP_INVALID;
    }

    //实例指针
    SimpleIOCPPipeCenter *m_pThis;

    //操作类型
    TOperatationType m_opType;

    //管道
    HANDLE m_hPipe;

    //用户自定义数据
    void *m_pUserData;

    //缓存总大小
    uint32_t m_nBufferSize;

    //实际数据总大小
    uint32_t m_nDataSize;

    //数据包头长度, 接收时候用
    uint32_t m_nHeaderSize;

    //传输完成的数据大小
    uint32_t m_nCompletedSize;

    //数据缓冲区
    uint8_t *m_pBuffer;
};

SimpleIOCPPipeCenter::SimpleIOCPPipeCenter()
{
    if (0 == g_allocationGranularity)
    {
        SYSTEM_INFO sysInfo;
        ::GetSystemInfo(&sysInfo);
        g_allocationGranularity = sysInfo.dwAllocationGranularity;
    }
}

SimpleIOCPPipeCenter::~SimpleIOCPPipeCenter()
{
    CloseAllPipes();
}

IOCPThreadPool &SimpleIOCPPipeCenter::GetThreadPool(DWORD dwNumOfRun /*= 0*/,
                                                    DWORD dwNumOfMax /*= 0*/)
{
    if (!m_threadPool)
    {
        //加锁,确保线程安全
        ATL::CCritSecLock lock{m_lock.m_sec};
        m_threadPool.CreateIOCPThreadPool(dwNumOfRun, dwNumOfMax);
    }
    return m_threadPool;
}

HANDLE SimpleIOCPPipeCenter::CreatePipe(std::wstring &name)
{
    if (name.empty())
    {
        name = LR"(\\.\pipe\)" + boost::uuids::to_wstring(boost::uuids::random_generator()());
    }
    HANDLE hPipe = ::CreateNamedPipe(name.c_str(),
                                     PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED |
                                         FILE_FLAG_FIRST_PIPE_INSTANCE | WRITE_OWNER,
                                     PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
                                     1,
                                     (DWORD)g_allocationGranularity,
                                     (DWORD)g_allocationGranularity,
                                     0,
                                     nullptr);
    if ((hPipe == nullptr) || (hPipe == INVALID_HANDLE_VALUE))
    {
        name.clear();
        return nullptr;
    }
    if (!GetThreadPool().BindDeviceToIOCPThreadPool(hPipe, OnIOCompleted))
    {
        ::CloseHandle(hPipe);
        name.clear();
        return nullptr;
    }
    ATL::CCritSecLock lock{m_lock.m_sec};
    m_pipePool[hPipe] = name;
    return hPipe;
}

HANDLE SimpleIOCPPipeCenter::ConnectToServer(const std::wstring &name)
{
    HANDLE hPipe = ::CreateFile(name.c_str(),
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                NULL);
    if ((hPipe == NULL) || (hPipe == INVALID_HANDLE_VALUE))
    {
        return nullptr;
    }
    if (!GetThreadPool().BindDeviceToIOCPThreadPool(hPipe, OnIOCompleted))
    {
        ::CloseHandle(hPipe);
        return nullptr;
    }
    ATL::CCritSecLock lock{m_lock.m_sec};
    m_pipePool[hPipe] = name;
    return hPipe;
}

std::wstring SimpleIOCPPipeCenter::GetPipeName(HANDLE hPipe)
{
    ATL::CCritSecLock lock{m_lock.m_sec};
    auto it = m_pipePool.find(hPipe);
    if (it != m_pipePool.end())
    {
        return it->second;
    }
    return L"";
}

bool SimpleIOCPPipeCenter::AsyncWaitForClientConnect(HANDLE hPipe, void *pUserData)
{
    if (!CheckPipe(hPipe))
    {
        return false;
    }
    auto pOvlp = AllocOverLapped(0);
    pOvlp->m_pThis = this;
    pOvlp->m_opType = TOperatationType::OP_CONNECT;
    pOvlp->m_hPipe = hPipe;
    pOvlp->m_pUserData = pUserData;
    if (!::ConnectNamedPipe(hPipe, pOvlp) && (::GetLastError() != ERROR_IO_PENDING))
    {
        assert(!"AsyncWaitForClientConnect失败");
        m_privateHeap.Free(pOvlp);
        return false;
    }
    return true;
}

SimpleIOCPPipeCenter::SendBuffer SimpleIOCPPipeCenter::GetSendBuffer(uint32_t nDataLength)
{
    SendBuffer buffer;
    buffer.m_pOvlp = AllocOverLapped(nDataLength);
    return std::move(buffer);
}

bool SimpleIOCPPipeCenter::AsyncSend(HANDLE hPipe, SendBuffer &&buffer, void *pUserData)
{
    if (!CheckPipe(hPipe))
    {
        return false;
    }
    auto pOvlp = buffer.m_pOvlp;
    buffer.m_pOvlp = nullptr;
    pOvlp->m_pThis = this;
    pOvlp->m_opType = TOperatationType::OP_SEND;
    pOvlp->m_hPipe = hPipe;
    pOvlp->m_pUserData = pUserData;
    if (!::WriteFile(hPipe, pOvlp->m_pBuffer, pOvlp->m_nDataSize, NULL, pOvlp) &&
        (::GetLastError() != ERROR_IO_PENDING))
    {
        assert(!"AsyncSend失败");
        m_privateHeap.Free(pOvlp);
        return false;
    }
    return true;
}

bool SimpleIOCPPipeCenter::AsyncReceive(HANDLE hPipe, uint32_t nHeaderSize, void *pUserData)
{
    if (nHeaderSize == 0)
    {
        assert(!"包头长度不能为0");
        return false;
    }
    if (!CheckPipe(hPipe))
    {
        return false;
    }
    //预先给4096的缓冲区，减少二次分配的机率
    auto pOvlp = AllocOverLapped(4096 > nHeaderSize ? 4096 : nHeaderSize);
    pOvlp->m_pThis = this;
    pOvlp->m_opType = TOperatationType::OP_RECEIVE;
    pOvlp->m_hPipe = hPipe;
    pOvlp->m_pUserData = pUserData;
    pOvlp->m_nHeaderSize = nHeaderSize;
    //先接收包头
    if (!::ReadFile(hPipe, pOvlp->m_pBuffer, nHeaderSize, NULL, pOvlp) &&
        (::GetLastError() != ERROR_IO_PENDING))
    {
        assert(!"AsyncReceive失败");
        m_privateHeap.Free(pOvlp);
        return false;
    }
    return true;
}

void SimpleIOCPPipeCenter::ClosePipe(HANDLE hPipe)
{
    if ((hPipe != nullptr) && (hPipe != INVALID_HANDLE_VALUE))
    {
        ATL::CCritSecLock lock{m_lock.m_sec};
        auto it = m_pipePool.find(hPipe);
        if (it != m_pipePool.end())
        {
            ::CloseHandle(hPipe);
            m_pipePool.erase(it);
        }
    }
}

void SimpleIOCPPipeCenter::CloseAllPipes()
{
    if (m_threadPool)
    {
        m_threadPool.CloseIOCPThreadPool();
    }
    for (auto it = m_pipePool.begin(); it != m_pipePool.end(); ++it)
    {
        ::CloseHandle(it->first);
    }
    m_pipePool.clear();
    if (m_privateHeap.m_hHeap)
    {
        ::HeapDestroy(m_privateHeap.Detach());
    }
}

bool SimpleIOCPPipeCenter::CheckPipe(HANDLE hPipe)
{
    if ((hPipe == nullptr) || (hPipe == INVALID_HANDLE_VALUE))
    {
        assert(!"invalid handle");
        return false;
    }
#ifdef _DEBUG
    ATL::CCritSecLock lock{m_lock.m_sec};
    auto it = m_pipePool.find(hPipe);
    if (it == m_pipePool.end())
    {
        assert(!"invalid handle");
        return false;
    }
#endif // _DEBUG
    return true;
}

SimpleIOCPPipeCenter::TPipeOverLapped *SimpleIOCPPipeCenter::AllocOverLapped(uint32_t nDataLength)
{
    if (m_privateHeap.m_hHeap == NULL)
    {
        ATL::CCritSecLock lock{m_lock.m_sec};
        if (m_privateHeap.m_hHeap == NULL)
        {
            m_privateHeap.Attach(::HeapCreate(0, 0, 0), true);
        }
    }
    uint32_t nBufferSize = (sizeof(TPipeOverLapped) + nDataLength + 256) / 256 * 256;
    TPipeOverLapped *pOvlp = (TPipeOverLapped *)m_privateHeap.Allocate(nBufferSize);
    assert(pOvlp);
    ::new (pOvlp) TPipeOverLapped();
    pOvlp->m_nBufferSize = nBufferSize - sizeof(TPipeOverLapped);
    pOvlp->m_pBuffer = (uint8_t *)(pOvlp + 1);
    return pOvlp;
}

bool SimpleIOCPPipeCenter::ReallocOverLapped(TPipeOverLapped *&pOvlp, uint32_t nNewDataLength)
{
    if (!pOvlp)
    {
        pOvlp = AllocOverLapped(nNewDataLength);
        return (pOvlp != nullptr);
    }
    else if (pOvlp->m_nBufferSize < nNewDataLength)
    {
        if (m_privateHeap.m_hHeap == NULL)
        {
            ATL::CCritSecLock lock{m_lock.m_sec};
            if (m_privateHeap.m_hHeap == NULL)
            {
                m_privateHeap.Attach(::HeapCreate(0, 0, 0), true);
            }
        }
        uint32_t nBufferSize = (sizeof(TPipeOverLapped) + nNewDataLength + 256) / 256 * 256;
        auto pNewOvlp = (TPipeOverLapped *)m_privateHeap.Reallocate(pOvlp, nBufferSize);
        if (!pNewOvlp)
        {
            return false;
        }
        //内存已重新分配, 指针值更新
        pOvlp = pNewOvlp;
        pOvlp->m_nBufferSize = nBufferSize - sizeof(TPipeOverLapped);
        pOvlp->m_pBuffer = (uint8_t *)(pOvlp + 1);
    }
    return true;
}

void SimpleIOCPPipeCenter::OnIOCompleted(DWORD dwErrno,
                                         DWORD dwNumberOfBytesTransferred,
                                         LPOVERLAPPED pOverlapped)
{
    assert(pOverlapped);
    if (!pOverlapped)
    {
        return;
    }
    TPipeOverLapped *pOvlp = (TPipeOverLapped *)pOverlapped;
    assert(pOvlp->m_pThis);
    pOvlp->m_pThis->OnIOCompletedImpl(dwErrno, dwNumberOfBytesTransferred, pOvlp);
}

void SimpleIOCPPipeCenter::OnIOCompletedImpl(DWORD dwErrno,
                                             DWORD dwNumberOfBytesTransferred,
                                             TPipeOverLapped *pOvlp)
{
    bool bSuccess = (dwErrno == ERROR_SUCCESS);
    switch (pOvlp->m_opType)
    {
    case TOperatationType::OP_CONNECT:
        OnClientConnected(bSuccess, pOvlp->m_hPipe, pOvlp->m_pUserData);
        break;
    case TOperatationType::OP_SEND:
        if (bSuccess)
        {
            pOvlp->m_nCompletedSize += dwNumberOfBytesTransferred;
            assert(pOvlp->m_nCompletedSize <= pOvlp->m_nDataSize);
            if (pOvlp->m_nCompletedSize < pOvlp->m_nDataSize)
            {
                if (::WriteFile(pOvlp->m_hPipe,
                                pOvlp->m_pBuffer + pOvlp->m_nCompletedSize,
                                pOvlp->m_nDataSize - pOvlp->m_nCompletedSize,
                                nullptr,
                                pOvlp) ||
                    (::GetLastError() == ERROR_IO_PENDING))
                {
                    //继续发送，不回调，不回收
                    return;
                }
                else
                {
                    assert(!"WriteFile失败");
                    bSuccess = false;
                }
            }
        }
        OnSend(bSuccess,
               pOvlp->m_hPipe,
               pOvlp->m_pBuffer,
               pOvlp->m_nDataSize,
               pOvlp->m_nCompletedSize,
               pOvlp->m_pUserData);
        break;
    case TOperatationType::OP_RECEIVE:
        if (bSuccess)
        {
            pOvlp->m_nCompletedSize += dwNumberOfBytesTransferred;
            uint32_t nLeftSize = 0;
            if (pOvlp->m_nCompletedSize < pOvlp->m_nHeaderSize)
            {
                //包头接收不完整
                nLeftSize = pOvlp->m_nHeaderSize - pOvlp->m_nCompletedSize;
                assert(pOvlp->m_nDataSize == 0);
            }
            else
            {
                if (pOvlp->m_nDataSize == 0)
                {
                    //从包头中解析出数据总长
                    assert(pOvlp->m_nHeaderSize == pOvlp->m_nCompletedSize);
                    pOvlp->m_nDataSize = pOvlp->m_pThis->GetDataLengthFromHeader(
                        pOvlp->m_hPipe, pOvlp->m_pBuffer, pOvlp->m_nHeaderSize, pOvlp->m_pUserData);
                }
                //计数剩余长度
                assert(pOvlp->m_nDataSize >= pOvlp->m_nCompletedSize);
                nLeftSize = pOvlp->m_nDataSize - pOvlp->m_nCompletedSize;
            }

            if (nLeftSize > 0)
            {
                if (nLeftSize > pOvlp->m_nBufferSize - pOvlp->m_nCompletedSize)
                {
                    //缓存不够
                    bSuccess = pOvlp->m_pThis->ReallocOverLapped(
                        pOvlp, pOvlp->m_nCompletedSize + nLeftSize);
                }
                if (bSuccess)
                {
                    //接收剩余的数据
                    bSuccess = (::ReadFile(pOvlp->m_hPipe,
                                           pOvlp->m_pBuffer + pOvlp->m_nCompletedSize,
                                           nLeftSize,
                                           nullptr,
                                           pOvlp) ||
                                (::GetLastError() == ERROR_IO_PENDING));
                    if (bSuccess)
                    {
                        return;
                    }
                    assert(!"ReadFile失败");
                }
            }
        }
        OnReceive(bSuccess,
                  pOvlp->m_hPipe,
                  pOvlp->m_pBuffer,
                  pOvlp->m_nCompletedSize,
                  pOvlp->m_pUserData);
        break;
    default:
        assert(0);
        break;
    }
    m_privateHeap.Free(pOvlp);
}

SimpleIOCPPipeCenter::SendBuffer::SendBuffer()
{
    m_pOvlp = nullptr;
}

SimpleIOCPPipeCenter::SendBuffer::SendBuffer(SimpleIOCPPipeCenter::SendBuffer &&other)
{
    m_pOvlp = other.m_pOvlp;
    other.m_pOvlp = nullptr;
}

SimpleIOCPPipeCenter::SendBuffer::~SendBuffer()
{
    if (m_pOvlp)
    {
        assert(!"SendBuffer内存泄漏");
    }
}

bool SimpleIOCPPipeCenter::SendBuffer::AddData(const void *pData, uint32_t nDataLength)
{
    assert(m_pOvlp);
    assert(m_pOvlp->m_nBufferSize - m_pOvlp->m_nDataSize >= nDataLength);
    if (m_pOvlp && (m_pOvlp->m_nBufferSize - m_pOvlp->m_nDataSize >= nDataLength))
    {
        std::memcpy(m_pOvlp->m_pBuffer + m_pOvlp->m_nDataSize, pData, nDataLength);
        m_pOvlp->m_nDataSize += nDataLength;
        return true;
    }
    return false;
}

SHARELIB_END_NAMESPACE
