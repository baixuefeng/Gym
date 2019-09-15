#pragma once

#include <codecvt>
#include <ostream>
#include <string>

/* 包含该头件后将改变标准库中流输出的行为：
    字符类型不相同时，使用流里面的local进行字符转码，而后输出。
*/

namespace std {
template<class _Traits>
std::basic_ostream<char, _Traits> &operator<<(std::basic_ostream<char, _Traits> &os,
                                              const wchar_t *pStr)
{
    auto &fct = std::use_facet<std::codecvt_utf16<wchar_t>>(os.getloc());
    std::wstring_convert<std::remove_reference_t<decltype(fct)>> cvt(&fct);
    return os << cvt.to_bytes(pStr);
}

template<class _Traits1, class _Traits2, class _Alloc>
std::basic_ostream<char, _Traits1> &operator<<(
    std::basic_ostream<char, _Traits1> &os,
    const std::basic_string<wchar_t, _Traits2, _Alloc> &wstr)
{
    auto &fct = std::use_facet<std::codecvt_utf16<wchar_t>>(os.getloc());
    std::wstring_convert<std::remove_reference_t<decltype(fct)>> cvt(&fct);
    return os << cvt.to_bytes(wstr);
}

template<class _Traits>
std::basic_ostream<wchar_t, _Traits> &operator<<(std::basic_ostream<wchar_t, _Traits> &os,
                                                 const char *pStr)
{
    auto &fct = std::use_facet<std::codecvt_utf16<wchar_t>>(os.getloc());
    std::wstring_convert<std::remove_reference_t<decltype(fct)>> cvt(&fct);
    return os << cvt.from_bytes(pStr);
}

template<class _Traits1, class _Traits2, class _Alloc>
std::basic_ostream<wchar_t, _Traits1> &operator<<(
    std::basic_ostream<wchar_t, _Traits1> &os,
    const std::basic_string<char, _Traits2, _Alloc> &str)
{
    auto &fct = std::use_facet<std::codecvt_utf16<wchar_t>>(os.getloc());
    std::wstring_convert<std::remove_reference_t<decltype(fct)>> cvt(&fct);
    return os << cvt.from_bytes(str);
}
} // namespace std
