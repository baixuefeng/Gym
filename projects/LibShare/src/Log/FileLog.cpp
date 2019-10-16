#include "targetver.h"
#ifndef _CRT_SECURE_NO_WARNINGS
#    define _CRT_SECURE_NO_WARNINGS
#endif
#include "Log/FileLog.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <locale>
#include <mutex>
#include <boost/date_time.hpp>
#include <boost/dll.hpp>
#include <atlmem.h>
#include <process.h>
#include <shellapi.h>
#include <strsafe.h>
#include <windows.h>
#include "LogFileManager.h"
#include "Other/VersionTime.h"
#include "Thread/lockfree_queue.h"

SHARELIB_BEGIN_NAMESPACE

static bool g_bOneLogPerProcess = false;

void SetGlobalOneLogPerProcess(bool bOneLogPerProcess)
{
    g_bOneLogPerProcess = bOneLogPerProcess;
}

struct LogBuffer
{
    char *m_pBuffer = nullptr;
    size_t m_nBufSize = 0;
    size_t m_nLoglength = 0;
};

enum class OP_TYPE
{
    WRITE_LOG,
    DELETE_TODAY_BEFORE,
    DELETE_ALL,
    QUIT_LOG,
};

struct _LogCache
{
    OP_TYPE m_opType = OP_TYPE::WRITE_LOG;
    LogBuffer m_log;
};

struct FileLog::TImpl
{
    TImpl()
        : m_memPool(0, 0){};

    ~TImpl()
    {
        if (m_hThread) {
            ::CloseHandle(m_hThread);
            m_hThread = nullptr;
        }
    }

    void AddRef() { ++m_refCount; }

    void Release()
    {
        if (--m_refCount == 0) {
            delete this;
        }
    }

    bool Init()
    {
        if (!m_bInitOK) {
            std::call_once(m_initFlag, [this]() {
                m_bInitOK = m_logFileManager.SetLogPathAndName(
                    m_paramPath.c_str(), m_paramName.c_str(), m_bOneLogPerProcess);
            });
        }
        return m_bInitOK;
    }

    bool Start()
    {
        if (!Init()) {
            return false;
        }
        std::lock_guard<decltype(m_handleLock)> lock(m_handleLock);
        if (NULL == m_hThread) {
            m_hThread = (HANDLE)_beginthreadex(NULL, 0, TImpl::ThreadFunc, this, 0, NULL);
            if (NULL == m_hThread) {
                char *pszErrMsg = NULL;
                ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                     FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL,
                                 GetLastError(),
                                 0,
                                 (LPSTR)(&pszErrMsg),
                                 0,
                                 NULL);
                std::string strCombMsg = std::string("创建日志线程失败，原因：") +
                                         std::string(pszErrMsg);
                ::LocalFree(pszErrMsg);
                m_logFileManager.Write(strCombMsg.c_str(), strCombMsg.size());
                return false;
            } else {
                AddRef();
            }
        }
        return true;
    }

    LogBuffer GetLogBuffer(size_t nSize)
    {
        LogBuffer buffer;
        char *p = (char *)m_memPool.Allocate(nSize);
        if (p) {
            buffer.m_pBuffer = p;
            buffer.m_nBufSize = nSize;
            std::memset(p, 0, nSize);
        }
        return buffer;
    }

    bool WriteLog(const LogBuffer &log)
    {
        if (!Init()) {
            return false;
        }
        if ((NULL == log.m_pBuffer) || (0 == log.m_nLoglength)) {
            if (NULL != log.m_pBuffer) {
                m_memPool.Free(log.m_pBuffer);
            }
            return true;
        }

        if (!m_hThread && !Start()) {
            assert(!"日志线程启动失败");
            return false;
        }

        _LogCache temp;
        temp.m_log = log;
        m_logQueue.push(temp);
        return true;
    }

    void Stop()
    {
        if (m_hThread && ::WaitForSingleObject(m_hThread, 0) == WAIT_OBJECT_0) {
            Release();
        } else if (m_hThread) {
            _LogCache temp;
            temp.m_opType = OP_TYPE::QUIT_LOG;
            m_logQueue.push(temp);
            ::WaitForSingleObject(m_hThread, 200);
        }
    }

    static unsigned int __stdcall ThreadFunc(void *pVoid)
    {
        TImpl *pThis = static_cast<TImpl *>(pVoid);
        auto &cvtFacet = std::use_facet<boost::filesystem::path::codecvt_type>(std::locale());
        std::wstring_convert<std::remove_reference_t<decltype(cvtFacet)>> cvt(&cvtFacet);
        _LogCache oneLog;

        {
            {
                SYSTEMTIME sysTime{};
                ::GetLocalTime(&sysTime);

                char szBuff[50]{};
                ::StringCbPrintfA(szBuff,
                                  sizeof(szBuff),
                                  "[log start][%I64u][%02hu:%02hu:%02hu.%03hu]\n",
                                  std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::high_resolution_clock::now().time_since_epoch())
                                      .count(),
                                  sysTime.wHour,
                                  sysTime.wMinute,
                                  sysTime.wSecond,
                                  sysTime.wMilliseconds);
                pThis->m_logFileManager.Write(szBuff);
            }

            boost::system::error_code ec;
            auto programPath = boost::dll::program_location(ec);
            if (!ec) {
                pThis->m_logFileManager.Write("exe: ");
                pThis->m_logFileManager.Write(programPath.string().c_str());
                auto strVer = GetModuleVersion(programPath.wstring().c_str());
                if (!strVer.empty()) {
                    pThis->m_logFileManager.Write(", version:");
                    pThis->m_logFileManager.Write(cvt.to_bytes(strVer).c_str());
                }
                pThis->m_logFileManager.Write("\n");
            }

            auto modulePath = boost::dll::this_line_location(ec);
            if (!ec && programPath != modulePath) {
                pThis->m_logFileManager.Write("module: ");
                pThis->m_logFileManager.Write(modulePath.string().c_str());
                auto strVer = GetModuleVersion(modulePath.wstring().c_str());
                if (!strVer.empty()) {
                    pThis->m_logFileManager.Write(", version:");
                    pThis->m_logFileManager.Write(cvt.to_bytes(strVer).c_str());
                }
                pThis->m_logFileManager.Write("\n");
            }
        }
        for (;;) {
            if (!pThis->m_logQueue.try_pop(oneLog)) {
                //缓冲区已空
                pThis->m_logFileManager.Flush();
                while (!pThis->m_logQueue.try_pop(oneLog)) {
                    ::Sleep(30);
                }
            }

            if (oneLog.m_opType == OP_TYPE::WRITE_LOG) {
                if (0 != oneLog.m_log.m_nLoglength) {
                    pThis->m_logFileManager.Write(oneLog.m_log.m_pBuffer,
                                                  oneLog.m_log.m_nLoglength);
                }
                pThis->m_memPool.Free(oneLog.m_log.m_pBuffer);
            } else if (oneLog.m_opType == OP_TYPE::QUIT_LOG) {
                break;
            } else {
                pThis->m_logFileManager.DeleteLogFile(oneLog.m_opType == OP_TYPE::DELETE_ALL);
            }
        }

        pThis->m_logFileManager.Write("[log end]\n");
        pThis->Release();
        return 0;
    }

    std::atomic<size_t> m_refCount{0};
    std::string m_paramPath;          //路径参数
    std::string m_paramName;          //名字参数
    bool m_bOneLogPerProcess = false; //进程模型参数
    LogFileManager m_logFileManager;
    lockfree_queue<_LogCache> m_logQueue;
    ATL::CWin32Heap m_memPool;
    bool m_bInitOK = false;
    std::once_flag m_initFlag;
    HANDLE m_hThread = nullptr;
    std::mutex m_handleLock;
};

//--------------------------------------------------------------------------------------

FileLogOstream::FileLogOstream(FileLog *pLog,
                               size_t bufLen,
                               bool bTime /*= false*/,
                               int nLine /*= 0*/)
    : m_pLog(pLog)
    , std::ostream(&m_buf)
{
    if (!m_pLog) {
        static FileLog *s_pGlobalLog = nullptr;
        static std::once_flag s_globalInitFlag;
        if (!s_pGlobalLog) {
            std::call_once(s_globalInitFlag, []() {
                static FileLog s_log{nullptr, nullptr, g_bOneLogPerProcess};
                s_pGlobalLog = &s_log;
            });
        }
        m_pLog = s_pGlobalLog;
    }
    if (!m_pLog) {
        return;
    }
    auto logbuf = m_pLog->m_pImpl->GetLogBuffer(bufLen);
    if (!logbuf.m_pBuffer) {
        return;
    }
    m_buf.pubsetbuf(logbuf.m_pBuffer, logbuf.m_nBufSize);
    {
        //static std::once_flag s_localFlags;
        //static std::locale * s_pLocal = nullptr;
        //std::call_once(s_localFlags,
        //    []()
        //{
        //    //构造local很耗时
        //    static std::locale s_loc{ std::locale(""), std::locale::classic(), std::locale::ctype };
        //    s_pLocal = &s_loc;
        //});
        //if (s_pLocal)
        //{
        //    imbue(*s_pLocal);
        //}
    }
    if (bTime) {
        Printf("[%I64u]",
               std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::high_resolution_clock::now().time_since_epoch())
                   .count());
    } else {
        //用<<输出time_of_day速度较慢
        auto &&timeOfDay = boost::posix_time::microsec_clock::local_time().time_of_day();
        Printf("[%02I64d:%02I64d:%02I64d.%06I64d]",
               timeOfDay.hours(),
               timeOfDay.minutes(),
               timeOfDay.seconds(),
               timeOfDay.fractional_seconds());
    }
    if (nLine > 0) {
        Printf("[Tid:%u][L:%u]", ::GetCurrentThreadId(), nLine);
    }
}

FileLogOstream::~FileLogOstream()
{
    if (m_pLog && m_buf.get_buffer()) {
        LogBuffer logbuf;
        logbuf.m_pBuffer = m_buf.get_buffer();
        logbuf.m_nBufSize = (size_t)m_buf.get_buffer_size();
        logbuf.m_nLoglength = (size_t)tellp();
        m_pLog->m_pImpl->WriteLog(logbuf);
    }
}

FileLogOstream &FileLogOstream::Printf(const char *pFmt, ...)
{
    if (m_pLog && m_buf.get_buffer() && (m_buf.get_buffer_size() > tellp())) {
        va_list vaParam;
        va_start(vaParam, pFmt);
        char *pEnd = nullptr;
        ::StringCchVPrintfExA(m_buf.get_buffer() + (std::streamoff)tellp(),
                              (size_t)(m_buf.get_buffer_size() - tellp()),
                              &pEnd,
                              nullptr,
                              0,
                              pFmt,
                              vaParam);
        seekp(pEnd - m_buf.get_buffer());
        va_end(vaParam);
    }
    return *this;
}

//---------------------------------------------------------------------------

FileLog::FileLog(const char *pPath /*= nullptr*/,
                 const char *pName /*= nullptr*/,
                 bool bOneLogPerProcess /*= false*/)
{
    m_pImpl = new TImpl;
    m_pImpl->AddRef();

    if (pPath != nullptr) {
        m_pImpl->m_paramPath = pPath;
    }
    if (pName != nullptr) {
        m_pImpl->m_paramName = pName;
    }
    m_pImpl->m_bOneLogPerProcess = bOneLogPerProcess;
}

FileLog::~FileLog()
{
    m_pImpl->Stop();
    m_pImpl->Release();
}

void FileLog::DeleteLogFile(bool bDeleteAll)
{
    if (m_pImpl->Init()) {
        if (!m_pImpl->m_hThread && !m_pImpl->Start()) {
            return;
        } else {
            _LogCache oneLog;

            if (bDeleteAll) {
                oneLog.m_opType = OP_TYPE::DELETE_ALL;
            } else {
                oneLog.m_opType = OP_TYPE::DELETE_TODAY_BEFORE;
            }
            m_pImpl->m_logQueue.push(oneLog);
        }
    }
}

void FileLog::ShowLogDir()
{
    if (m_pImpl->Init()) {
        std::string strLogPath = m_pImpl->m_logFileManager.GetLogFullPath();
        std::string::size_type uPos = strLogPath.find_last_of('\\');
        ::ShellExecuteA(NULL, "open", strLogPath.substr(0, uPos).c_str(), NULL, NULL, SW_SHOW);
    }
}

void FileLog::ShowLogFile()
{
    if (m_pImpl->Init()) {
        ::ShellExecuteA(
            NULL, "open", m_pImpl->m_logFileManager.GetLogFullPath().c_str(), NULL, NULL, SW_SHOW);
    }
}

SHARELIB_END_NAMESPACE
