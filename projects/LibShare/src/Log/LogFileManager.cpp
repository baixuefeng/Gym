#include "targetver.h"
#ifndef _CRT_SECURE_NO_WARNINGS
#    define _CRT_SECURE_NO_WARNINGS
#endif
#include "LogFileManager.h"
#include <algorithm>
#include <cstring>
#include <atlbase.h>
#include <atlconv.h>
#include <strsafe.h>
#include "Other/TraversalTree.h"

SHARELIB_BEGIN_NAMESPACE

LogFileManager::LogFileManager()
{
    m_bRunOK = false;
    m_bNeedFlush = false;
    m_pFile = nullptr;
}

LogFileManager::~LogFileManager()
{
    if (m_pFile) {
        std::fclose(m_pFile);
        m_pFile = nullptr;
    }
}

const char *LogFileManager::GetErrMsg()
{
    return m_strErrorMsg.c_str();
}

static HMODULE GetThisModule()
{
    HMODULE hModule = nullptr;
    ::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)&GetThisModule, &hModule);
    return hModule;
}

bool LogFileManager::SetLogPathAndName(const char *pPath,
                                       const char *pName,
                                       bool bOneLogPerProcess /*= false*/)
{
    {
        char szModleFullPath[MAX_PATH + 20]{};
        ::GetModuleFileNameA(GetThisModule(), szModleFullPath, MAX_PATH);
        //查找可执行文件名的开始位置
        char *pModuleName = std::strrchr(szModleFullPath, '\\');
        if (!pModuleName) {
            return false;
        }
        ++pModuleName;

        //查找可执行文件后缀的开始位置
        char *pDot = std::strrchr(szModleFullPath, '.');
        if (!pDot) {
            return false;
        }

        if ((nullptr == pPath) || (0 == std::strlen(pPath)) || std::strlen(pPath) > MAX_PATH) {
            m_strFilePath.assign(szModleFullPath, pModuleName);
        } else {
            m_strFilePath = pPath;
            if (m_strFilePath.back() != '\\') {
                m_strFilePath += '\\';
            }
        }
        //记录文件名
        m_strFilePath += "log\\";
        if (pName == nullptr || std::strlen(pName) == 0) {
            m_strFileName.assign(pModuleName, pDot);
        } else {
            m_strFileName = pName;
        }
        if (bOneLogPerProcess) {
            m_strFileName += std::string("_") + std::to_string(::GetCurrentProcessId());
        }
    }

    {
        //创建日志文件夹
        if (!::CreateDirectoryA(m_strFilePath.c_str(), nullptr)) {
            DWORD dwErr = ::GetLastError();
            if (ERROR_ALREADY_EXISTS != dwErr) {
                return false;
            }
        }

        char szDate[20]{};
        SYSTEMTIME date{};
        ::GetLocalTime(&date);
        ::StringCbPrintfA(
            szDate, sizeof(szDate), "%4u%02u%02u", date.wYear, date.wMonth, date.wDay);
        if (!::CreateDirectoryA((m_strFilePath + szDate).c_str(), nullptr)) {
            DWORD dwErr = ::GetLastError();
            if (ERROR_ALREADY_EXISTS != dwErr) {
                return false;
            }
        }
        m_strFileFullName = m_strFilePath + szDate + "\\" + m_strFileName + ".log";
    }

    if (m_pFile) {
        std::clearerr(m_pFile);
        std::fclose(m_pFile);
        m_pFile = nullptr;
    }
    m_pFile = std::fopen(m_strFileFullName.c_str(), "abcS");
    //fopen_s这种方式打开的文件不能共享读
    if (!m_pFile) {
        return false;
    }
    m_bRunOK = true;
    return true;
}

bool LogFileManager::Write(const char *pkszMsg, size_t uiCount)
{
    if (!m_bRunOK) {
        return false;
    }

    //写入内容
    if (!uiCount) {
        std::fwrite(pkszMsg, std::strlen(pkszMsg), 1, m_pFile);
    } else {
        std::fwrite(pkszMsg, uiCount, 1, m_pFile);
    }
    m_bNeedFlush = true;
    if (std::ftell(m_pFile) < MAX_LOG_FILE_SIZE) {
        return true;
    }

    //关闭当前文件
    m_bRunOK = false;
    std::clearerr(m_pFile);
    std::fclose(m_pFile);
    m_pFile = nullptr;

    //创建文件夹
    char szDate[20]{};
    SYSTEMTIME curTime{};
    ::GetLocalTime(&curTime);
    ::StringCbPrintfA(
        szDate, sizeof(szDate), "%4u%02u%02u", curTime.wYear, curTime.wMonth, curTime.wDay);
    if (!::CreateDirectoryA((m_strFilePath + szDate).c_str(), nullptr)) {
        DWORD dwErr = GetLastError();
        if (ERROR_ALREADY_EXISTS != dwErr) {
            char *pMsg = nullptr;
            ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                             nullptr,
                             dwErr,
                             0,
                             LPSTR(&pMsg),
                             0,
                             nullptr);
            m_strErrorMsg = pMsg;
            ::LocalFree(pMsg);
            return false;
        }
    }

    //移动当前文件并改名
    char szTime[20]{};
    ::StringCbPrintfA(
        szTime, sizeof(szTime), "_%02u%02u%02u", curTime.wHour, curTime.wMinute, curTime.wSecond);
    //并非跨盘移动，所以速度是有保障的
    if (!::MoveFileA(m_strFileFullName.c_str(),
                     (m_strFilePath + szDate + "\\" + m_strFileName + szTime + ".log").c_str())) {
        DWORD dwErr = GetLastError();
        if (ERROR_ALREADY_EXISTS != dwErr) {
            char *pMsg = nullptr;
            ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                             nullptr,
                             dwErr,
                             0,
                             LPSTR(&pMsg),
                             0,
                             nullptr);
            m_strErrorMsg = pMsg;
            ::LocalFree(pMsg);
            return false;
        }
    }

    {
        std::lock_guard<decltype(m_lockFileFullPath)> lock(m_lockFileFullPath);
        m_strFileFullName = m_strFilePath + szDate + "\\" + m_strFileName +
                            ".log"; //移动成功，当前日志文件重定位
    }

    //打开新文件准备输入
    m_pFile = std::fopen(m_strFileFullName.c_str(), "abcS");
    //fopen_s这种方式打开的文件不能共享读
    if (!m_pFile) {
        DWORD dwErr = GetLastError();
        char *pMsg = nullptr;
        ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                             FORMAT_MESSAGE_IGNORE_INSERTS,
                         nullptr,
                         dwErr,
                         0,
                         LPSTR(&pMsg),
                         0,
                         nullptr);
        m_strErrorMsg = pMsg;
        ::LocalFree(pMsg);
        return false;
    }
    m_bRunOK = true;
    return true;
}

void LogFileManager::Flush()
{
    if (m_bNeedFlush) {
        if (m_pFile) {
            std::fflush(m_pFile);
        }
        m_bNeedFlush = false;
    }
}

const std::string &LogFileManager::GetLogFullPath() const
{
    std::lock_guard<decltype(m_lockFileFullPath)> lock(m_lockFileFullPath);
    return m_strFileFullName;
}

void LogFileManager::DeleteLogFile(bool bDeleteAll)
{
    std::lock_guard<decltype(m_lockFileFullPath)> lock(m_lockFileFullPath);
    std::wstring currLogFile = ATL::CA2W(m_strFileFullName.c_str(), CP_ACP).operator LPWSTR();
    std::wstring logToday = currLogFile.substr(0, currLogFile.find_last_of('\\'));
    std::wstring logDir = logToday.substr(0, logToday.find_last_of('\\'));

    if (bDeleteAll) {
        bool processCurrFile = false;
        ForEachItemInDirectory(
            logDir.c_str(),
            10,
            //参数3,lambda表达式
            [](const wchar_t *) -> bool { return true; },
            //参数4,lambda表达式
            [&currLogFile, &processCurrFile, this](const wchar_t *pFileName) {
                if (!processCurrFile && wcscmp(pFileName, currLogFile.c_str()) == 0) {
                    if (m_pFile) {
                        std::clearerr(m_pFile);
                        std::fclose(m_pFile);
                        m_pFile = nullptr;
                    }
                    m_pFile = std::fopen(m_strFileFullName.c_str(), "wbcS");
                    processCurrFile = true;
                } else {
                    ::DeleteFileW(pFileName);
                }
            },
            ::RemoveDirectoryW);
    } else {
        bool bTodaySkiped = false;
        ForEachItemInDirectory(
            logDir.c_str(),
            10,
            //参数3,lambda表达式
            [&logToday, &bTodaySkiped](const wchar_t *pDir) -> bool {
                if (!bTodaySkiped && wcscmp(pDir, logToday.c_str()) == 0) {
                    bTodaySkiped = true;
                    return false;
                } else {
                    return true;
                }
            },
            ::DeleteFileW,
            ::RemoveDirectoryW);
    }
}

SHARELIB_END_NAMESPACE
