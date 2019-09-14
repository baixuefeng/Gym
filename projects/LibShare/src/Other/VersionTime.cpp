#include "targetver.h"
#include "Other/VersionTime.h"
#include <vector>
#include <memory>
#include <windows.h>
#include <atlfile.h>
#include <strsafe.h>
#include <atlwinverapi.h>

//使用Version相关的函数需要引入的库
#pragma comment(lib, "Version.lib")

SHARELIB_BEGIN_NAMESPACE

// 文件名为空表示获取当前模块文件的时间。
// 三种类型的时间：创建时间，最后访问时间，修改时间
enum class TTimeType{ CREATE, LASTACCESS, MODIFY };

static bool GetLocaleFileTime(const wchar_t * pkszFileName, TTimeType emTimeType, SYSTEMTIME & stTime)
{
    ATL::CAtlFile file;
    HRESULT hr = E_FAIL;
    if ((pkszFileName == NULL) || (0 == std::wcslen(pkszFileName)))
    {
        //获取模块句柄
        HMODULE hModule = NULL;
        if (!::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            reinterpret_cast<LPCWSTR>(GetLocaleFileTime), &hModule))
        {
            return false;
        }
        wchar_t szName[MAX_PATH] = { 0 };
        ::GetModuleFileNameW(hModule, szName, MAX_PATH);

        hr = file.Create(szName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);
    }
    else
    {
        hr = file.Create(pkszFileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);
    }

    if (FAILED(hr))
    {
        return false;
    }

    //获取文件时间
    FILETIME stFTime1{}, stFTime2{};
    BOOL bRel = TRUE;
    switch (emTimeType)
    {
    case TTimeType::CREATE:
        bRel = ::GetFileTime(file, &stFTime1, NULL, NULL);
        break;
    case TTimeType::LASTACCESS:
        bRel = ::GetFileTime(file, NULL, &stFTime1, NULL);
        break;
    case TTimeType::MODIFY:
        bRel = ::GetFileTime(file, NULL, NULL, &stFTime1);
        break;
    }

    //时间转换
    bRel &= ::FileTimeToLocalFileTime(&stFTime1, &stFTime2);
    bRel &= ::FileTimeToSystemTime(&stFTime2, &stTime);
    return !!bRel;//消除编译器警告
}

//如果文件名为空，则获取当前模块文件的信息
static bool GetFileVersionInfoString(
    const wchar_t * pwkszFileName,
    TVersionKey emVersionKey,
    std::wstring & wstrVersionString)
{
    //选取文件名
    std::wstring strName;
    if ((pwkszFileName == NULL) || (0 == std::wcslen(pwkszFileName)))
    {
        strName.assign(MAX_PATH + 1, L'\0');
        HMODULE hModule = NULL;
        if (!::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            reinterpret_cast<LPCWSTR>(GetFileVersionInfoString), &hModule))
        {
            return false;
        }
        ::GetModuleFileNameW(hModule, &strName[0], MAX_PATH);
    }
    else
    {
        strName = pwkszFileName;
    }

    bool bXpOs = true; //XP系统
    std::vector<BYTE> byVersion;
    IFDYNAMICGETCACHEDFUNCTION(L"version.dll", GetFileVersionInfoSizeExW, pfnGetFileVersionInfoSizeExW)
    {
        IFDYNAMICGETCACHEDFUNCTION(L"version.dll", GetFileVersionInfoExW, pfnGetFileVersionInfoExW)
        {
            bXpOs = false;
            //XP以上系统需要使用Ex版本的函数,测试发现比如用非Ex的版本获取kernel32.dll的版本信息就是错误的.
            DWORD dwVersionBytes = pfnGetFileVersionInfoSizeExW(0, strName.c_str(), NULL);
            if (dwVersionBytes == 0)
            {
                return false;
            }
            byVersion.assign(dwVersionBytes, 0);
            if (!pfnGetFileVersionInfoExW(0, strName.c_str(), 0, dwVersionBytes, &byVersion[0]))
            {
                return false;
            }
        }
    }
    if (bXpOs)
    {
        //获取文件版本信息字节流
        DWORD dwVersionBytes = ::GetFileVersionInfoSizeW(strName.c_str(), NULL);
        if (dwVersionBytes == 0)
        {
            return false;
        }
        byVersion.assign(dwVersionBytes, 0);
        if (!::GetFileVersionInfoW(strName.c_str(), 0, dwVersionBytes, &byVersion[0]))
        {
            return false;
        }
    }

    if (byVersion.empty())
    {
        return false;
    }

    //获取版本信息代码页
    struct LANGANDCODEPAGE
    {
        WORD wLanguage;
        WORD wCodePage;
    } *pstLanguageCode = NULL;
    UINT uiTranslateBytes = 0;
    if (!::VerQueryValueW(&byVersion[0], L"\\VarFileInfo\\Translation",
        reinterpret_cast<LPVOID *>(&pstLanguageCode), &uiTranslateBytes))
    {
        return false;
    }

    //预先把可以使用的所有键名列出来，用枚举值参传，如果拼写不正确编译器报错。
    //相反如果直接通过字符串传参，拼写错误编译器不报错，却会导致运行时错误。
    static const wchar_t * VERSION_KEY_NAMES[] =
    {
        L"Comments",
        L"InternalName",
        L"OriginalFilename",
        L"ProductName",
        L"FileDescription",
        L"CompanyName",
        L"LegalCopyright",
        L"LegalTrademarks",
        L"SpecialBuild",
        L"PrivateBuild",
        L"FileVersion",
        L"ProductVersion"
    };

    //获取具体版本信息字符串
    wchar_t wszSubBlock[100] = { 0 };
    if (FAILED(::StringCbPrintfW(
        wszSubBlock,
        sizeof(wszSubBlock),
        L"\\StringFileInfo\\%04x%04x\\%s",
        pstLanguageCode[0].wLanguage,
        pstLanguageCode[0].wCodePage, //? 代码页的选择，如何支持多国语言
        VERSION_KEY_NAMES[(int)emVersionKey])))
    {
        return false;
    }
    wchar_t * pVersionInfo = NULL;//获取的版本信息只有UNICODE字符串，没有ANSI字符串
    UINT uiVersionChars = 0;
    if (!::VerQueryValueW(&byVersion[0], wszSubBlock,
        reinterpret_cast<LPVOID *>(&pVersionInfo), &uiVersionChars))
    {
        return false;
    }
    wstrVersionString = std::wstring(pVersionInfo, uiVersionChars);
    return true;
}

//----------------------------------------------------------------------------

std::wstring GetModuleModifyTime(const wchar_t *pModuleName)
{
    SYSTEMTIME stTime{};
    if (GetLocaleFileTime(pModuleName, TTimeType::MODIFY, stTime))
    {
        wchar_t szTime[100] = { 0 };
        ::StringCbPrintfW(szTime, sizeof(szTime),
            L"%04u-%02u-%02u %02u:%02u:%02u",
            stTime.wYear,
            stTime.wMonth,
            stTime.wDay,
            stTime.wHour,
            stTime.wMinute,
            stTime.wSecond);
        return szTime;
    }
    else
    {
        return L"";
    }
}

std::wstring GetModuleVersion(const wchar_t *pModuleName, TVersionKey key/* = TVersionKey::ProductVersion*/)
{
    std::wstring wstrTemp;
    if (GetFileVersionInfoString(pModuleName, key, wstrTemp))
    {
        return wstrTemp;
    }
    return L"";
}

std::wstring GetModuleProductName(const wchar_t *pModuleName)
{
    std::wstring wstrTemp;
    if (GetFileVersionInfoString(pModuleName, TVersionKey::ProductName, wstrTemp))
    {
        return wstrTemp;
    }
    return L"";
}

SHARELIB_END_NAMESPACE
