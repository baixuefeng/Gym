#pragma once

#include "MacroDefBase.h"
#include <string>

/*!
 * \file VersionTime.h
 */

SHARELIB_BEGIN_NAMESPACE

//----下面三个函数，如果错误，返回空字符串----------------------------------------

// 获取(如果参数为空)当前模块程序的修改时间，如果在动态库中使用，则获取动态库本身的修改时间
std::wstring GetModuleModifyTime(const wchar_t *pModuleName);

//版本信息可以看作是键值对，下面列出的是键
enum class TVersionKey
{
    Comments = 0,
    InternalName,
    OriginalFilename,
    ProductName,
    FileDescription,
    CompanyName,
    LegalCopyright,
    LegalTrademarks,
    SpecialBuild,
    PrivateBuild,
    FileVersion,
    ProductVersion
};

// 获取(如果参数为空)当前模块程序的版本号，如果在动态库中使用，则获取动态库本身的版本号
std::wstring GetModuleVersion(const wchar_t *pModuleName, TVersionKey key = TVersionKey::ProductVersion);

// 获取(如果参数为空)当前模块程序的内部产品名称，如果在动态库中使用，则获取动态库本身的内部产品名称
std::wstring GetModuleProductName(const wchar_t *pModuleName);

SHARELIB_END_NAMESPACE
