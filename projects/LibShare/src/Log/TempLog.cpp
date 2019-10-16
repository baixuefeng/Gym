#include "targetver.h"
#include "Log/TempLog.h"
#include <memory>
#include <mutex>
#include <boost/date_time.hpp>
#include <Windows.h>
#include <atlmem.h>
#include <strsafe.h>
#include "Log/SimpleAsyncSink.h"

SHARELIB_BEGIN_NAMESPACE

void InitConsole()
{
    struct AutoCloseFile
    {
        std::FILE *m_pConsoleOut = nullptr;
        std::FILE *m_pConsoleErr = nullptr;
        std::FILE *m_pConsoleIn = nullptr;

        ~AutoCloseFile()
        {
            if (m_pConsoleOut) {
                std::fclose(m_pConsoleOut);
            }
            if (m_pConsoleErr) {
                std::fclose(m_pConsoleErr);
            }
            if (m_pConsoleIn) {
                std::fclose(m_pConsoleIn);
            }
            ::FreeConsole();
        }
    };

    static std::once_flag s_consoleOnceFlag;
    static AutoCloseFile s_stdioFile;
    std::call_once(s_consoleOnceFlag, []() {
        ::AllocConsole();
        freopen_s(&s_stdioFile.m_pConsoleOut, "CONOUT$", "w", stdout);
        freopen_s(&s_stdioFile.m_pConsoleErr, "CONOUT$", "w", stderr);
        freopen_s(&s_stdioFile.m_pConsoleIn, "CONIN$", "r", stdin);
    });
}

//----------------------------------------------------------------------------

#define MAX_BUFFER_SIZE 1024
static ATL::CWin32Heap &GetGlobalMsgHeap()
{
    static ATL::CWin32Heap *s_pMsgHeap = NULL;
    static std::once_flag s_msgHeapOnceFlag;
    if (!s_pMsgHeap) {
        std::call_once(s_msgHeapOnceFlag, []() {
            static ATL::CWin32Heap s_msgHeap(0, 0);
            s_pMsgHeap = &s_msgHeap;
        });
    }
    return *s_pMsgHeap;
}

//----------------------------------------------------------------------------

ConsoleSafeOstream::ConsoleSafeOstream(bool bTime, int nLine)
    : std::wostream(&m_buf)
{
    if (!::GetStdHandle(STD_OUTPUT_HANDLE)) {
        return;
    }
    size_t nCount = MAX_BUFFER_SIZE;
    wchar_t *pBuffer = (wchar_t *)GetGlobalMsgHeap().Allocate(nCount * sizeof(wchar_t));
    if (pBuffer == NULL) {
        assert(0);
        return;
    }
    m_buf.pubsetbuf(pBuffer, nCount);
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
        //用<<输出time_of_day速度较慢
        auto &&timeOfDay = boost::posix_time::microsec_clock::local_time().time_of_day();
        Printf(L"[%02I64d:%02I64d:%02I64d.%06I64d]",
               timeOfDay.hours(),
               timeOfDay.minutes(),
               timeOfDay.seconds(),
               timeOfDay.fractional_seconds());
    }
    if (nLine > 0) {
        Printf(L"[Tid-%u][L-%u]", ::GetCurrentThreadId(), nLine);
    }
}

ConsoleSafeOstream::~ConsoleSafeOstream()
{
    if (m_buf.get_buffer()) {
        ::WriteConsoleW(::GetStdHandle(STD_OUTPUT_HANDLE),
                        m_buf.get_buffer(),
                        (DWORD)(tellp()),
                        nullptr,
                        nullptr);
        GetGlobalMsgHeap().Free(m_buf.get_buffer());
    }
}

ConsoleSafeOstream &ConsoleSafeOstream::Printf(const wchar_t *pFmt, ...)
{
    if (m_buf.get_buffer() && (m_buf.get_buffer_size() > tellp())) {
        va_list vaParam;
        va_start(vaParam, pFmt);
        wchar_t *pEnd = nullptr;
        ::StringCchVPrintfEx(m_buf.get_buffer() + (std::streamoff)tellp(),
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

//----------------------------------------------------------------------------

/* 
1.现象: 对ThreadSafeOstream重载 const string&, 而后调用 << int, 再调用 << string(), 就会失败;
   如果顺序相反就正确.
原因: 调用 << int之后, 类型变为 wostream, 找不到对const string&的重载版本了.
2.__VA_ARGS__
*/

SHARELIB_END_NAMESPACE
