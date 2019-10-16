#pragma once

#include <cstring>
#include <memory>
#include <string>
#include <windows.h>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

/** 遍历到子键时的处理函数
@param[in] path 注册表路径，只起记录的作用，不会依据此打开注册表
@param[in] keyName 子键的名字，不包含在path中
@param[out] samDesired 遍历子键时，用什么权限打开子键，默认为 KEY_READ
@return 返回true则遍历该子键，否则不遍历
*/
typedef bool (*PFProcessKeyFunc)(const std::wstring &path,
                                 const std::wstring &keyName,
                                 REGSAM &samDesired);

/** 遍历到值时的处理函数
@param[in] path 注册表路径，只起记录的作用，不会依据此打开注册表
@param[in] valueName 值的名字，不包含在path中
@param[in] dwValueType 值的类型
@param[in] dwDataBytes 数据的字节数
*/
typedef void (*PFProcessValueFunc)(const std::wstring &path,
                                   const std::wstring &valueName,
                                   DWORD dwValueType,
                                   const void *pValueData,
                                   DWORD dwDataBytes);

/** 遍历注册表
@param[in] path 注册表路径，只起记录的作用，不会依据此打开注册表
@param[in] hKey 要遍历的注册表键 用RegOpenKeyEx打开一个句柄
@param[in] nMaxDepth 最大遍历深度
@param[in] pfProcessKey 遍历到一个子键时的处理函数
@param[in] pfProcessValue 遍历到一个值时的处理函数
@return 返回系统错误码
*/
LSTATUS ForEachItemInTheRegKey(const std::wstring &path,
                               HKEY hKey,
                               unsigned int nMaxDepth,
                               PFProcessKeyFunc pfProcessKey,
                               PFProcessValueFunc pfProcessValue);

//-------------------------------------------------------------

/** 遍历目录，Op1必须是返回可判断bool值的操作算子，参数必须为 const wchar_t *,
    Op2和Op3 也是操作算子，参数必须为 const wchar_t *
@param[in] pDirPath 目录路径
@param[in] nMaxDepth 最大递归深度
@param[in] needRecursion 如果bool判断为true则对该目录递归遍历，否则跳过，入参为目录路径
@param[in] itemOp 对目录中的文件操作算子，入参为文件路径
@param[in] dirOp 对目录中的所有项处理完了之后，对该目录操作算子，入参为目录路径
@return 返回处理成功与否，Op1、Op2、Op3的返回值不影响函数的总体返回值
*/
template<class Op1, class Op2, class Op3>
bool ForEachItemInDirectory(const wchar_t *pDirPath,
                            unsigned int nMaxDepth,
                            Op1 needRecursion,
                            Op2 itemOp,
                            Op3 dirOp);

/** 删除目录中的所有文件
*/
#define DeleteDirectoryWithAllSubitems(dirPath)                                                    \
    ForEachItemInDirectory(                                                                        \
        dirPath,                                                                                   \
        UINT_MAX,                                                                                  \
        [](const wchar_t *) -> bool { return true; },                                              \
        ::DeleteFileW,                                                                             \
        ::RemoveDirectoryW)

///////////////////////////////////////////////////////////////////////////////////////////////

//----下面是模版函数的实现-------------------------------------

#pragma warning(push)
#pragma warning(disable : 4995)
#pragma warning(disable : 4996)
template<class Op1, class Op2, class Op3>
bool ForEachItemInDirectory(const wchar_t *pDirPath,
                            unsigned int nMaxDepth,
                            Op1 needRecursion,
                            Op2 itemOp,
                            Op3 dirOp)
{
    if (nMaxDepth == 0) {
        return true;
    }

    //检验合法性
    if (pDirPath == nullptr) {
        return false;
    }
    size_t nLenth = std::wcslen(pDirPath);
    if (nLenth >= MAX_PATH) {
        return false;
    }

    //预处理
    std::unique_ptr<wchar_t[]> spTargetDir(new wchar_t[MAX_PATH * 2]{});
    std::memcpy(spTargetDir.get(), pDirPath, nLenth * sizeof(wchar_t));
    if (pDirPath[nLenth - 1] != L'\\') {
        spTargetDir.get()[nLenth] = L'\\';
        spTargetDir.get()[nLenth + 1] = L'*';
        ++nLenth;
    } else {
        spTargetDir.get()[nLenth] = L'*';
    }

    //打开一个查找句柄
    WIN32_FIND_DATAW ffData;
    std::memset(&ffData, 0, sizeof(ffData));
    HANDLE hDir = ::FindFirstFileW(spTargetDir.get(), &ffData);
    if (hDir == INVALID_HANDLE_VALUE) {
        return false;
    }

    do {
        if (ffData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY &&
            std::wcscmp(ffData.cFileName, L".") != 0 && std::wcscmp(ffData.cFileName, L"..") != 0) {
            std::wcscpy(spTargetDir.get() + nLenth, ffData.cFileName);
            //递归处理子目录
            if ((nMaxDepth > 0) && needRecursion(spTargetDir.get())) {
                if (!ForEachItemInDirectory(
                        spTargetDir.get(), nMaxDepth - 1, needRecursion, itemOp, dirOp)) {
                    return false;
                }
            }
        } else if (ffData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
            std::wcscpy(spTargetDir.get() + nLenth, ffData.cFileName);
            //处理文件
            itemOp(spTargetDir.get());
        }
    } while (::FindNextFileW(hDir, &ffData));

    //处理最上层目录
    dirOp(pDirPath);
    ::FindClose(hDir);
    hDir = nullptr;
    return true;
}
#pragma warning(pop)

SHARELIB_END_NAMESPACE
