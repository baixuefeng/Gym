#pragma once
#include <exception>
#include <Windows.h>
#include "MacroDefBase.h"

// 请使用下面的宏，不要直接使用异常类及相关的函数

//[in]win32Func: const char*类型的win32函数名
//[in]dwErrno: DWORD类型的错误代码
#define THROW_WIN32_ERROR(win32Func, dwErrno)                                                      \
    {                                                                                              \
        shr::Win32Exception errMsg;                                                                \
        FormatExceptionMessage(errMsg, win32Func, dwErrno, __FUNCTION__, __LINE__);                \
        throw errMsg;                                                                              \
    }

#define THROW_SOCKET_ERROR(socketFunc, dwErrno)                                                    \
    {                                                                                              \
        shr::SocketException errMsg;                                                               \
        FormatExceptionMessage(errMsg, socketFunc, dwErrno, __FUNCTION__, __LINE__);               \
        throw errMsg;                                                                              \
    }

SHARELIB_BEGIN_NAMESPACE

struct Win32Exception : public std::exception
{
    Win32Exception(){};
    virtual const char *what() const { return m_szMsg; }
    virtual ~Win32Exception(){};
    char m_szMsg[256];
    DWORD m_dwErrno;
};

struct SocketException : public Win32Exception
{};

void FormatExceptionMessage(Win32Exception &errExcept,
                            const char *pWin32Func,
                            DWORD dwErrno,
                            const char *pInFunc,
                            int nLine);

SHARELIB_END_NAMESPACE
