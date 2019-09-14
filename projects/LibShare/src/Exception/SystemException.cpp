#include "targetver.h"
#include "Exception/SystemException.h"
#include <strsafe.h>

SHARELIB_BEGIN_NAMESPACE

void FormatExceptionMessage(Win32Exception & errExcept, const char *pWin32Func, DWORD dwErrno, const char *pInFunc, int nLine)
{
    errExcept.m_dwErrno = dwErrno;
    char szReason[200]{};
    ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErrno, 0, szReason, sizeof(szReason), NULL);
    char *pEnter = strstr(szReason, "\r\n");
    pEnter ? (*pEnter = '\0') : NULL;
    ::StringCbPrintfA(errExcept.m_szMsg, sizeof(errExcept.m_szMsg),
                      "(%s) error:%u(%s), in (%s), at L:%u.",
                      pWin32Func, dwErrno, szReason, pInFunc, nLine);
}

SHARELIB_END_NAMESPACE
