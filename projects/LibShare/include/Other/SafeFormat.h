#pragma once
#include <cassert>
#include <codecvt>
#include <locale>
#include <string>
#include <type_traits>
#include <boost/format.hpp>
#include "MacroDefBase.h"
#include "Other/OstreamCodeConvert.h"

SHARELIB_BEGIN_NAMESPACE

namespace detail {
template<class TChar>
void SafeFormatHelper(boost::basic_format<TChar> &)
{}

template<class TChar, class T1, class... T2>
void SafeFormatHelper(boost::basic_format<TChar> &formater, T1 &&param, T2 &&... args)
{
    formater % std::forward<T1>(param);
    SafeFormatHelper(formater, std::forward<T2>(args)...);
}
} // namespace detail

/** 安全的字符串格式化函数，内部实现是boost::format，可以兼容绝大部分C语言风格的
    格式化方法，另外还扩展了一些独有的格式化方法，详情见
    https://www.boost.org/doc/libs/1_68_0/libs/format/doc/format.html#printf_directives
    相比C的printf实现，安全性体现在几个方面：
        1. 缓冲区自动增长，避免了越界风险；
        2. 类型安全，按参数的真实类型输出，不会因为类型强转出错；
        3. 只要重载<<运算符，自定义类型就可以正确输出，比如std::string。
           如果不支持该类型(没有匹配的<<重载)，编译不过，会报错；而printf则不报错，直到运行时才出现莫名其妙的错误。
    与C的printf主要区别：
        1. C的printf中，如 %d,%u,%llu之类会忽略实际类型，强制认为一块内存为指定类型。
           这里的只是告诉说这是一个占位符，其实%d,%u并没有差别，一律按参数的真实类型进行输出，
           实质上在内部是用 << 进行输出。
        2. C的printf，如果参数个数与占位符数量不匹配，不一定出错。
           这里的必须严格匹配。
    字符集转换: 使用全局的local进行宽窄字符转换
@param[in] pFormat 格式化串
@param[in] ...args 参数
@return 如果出错，返回空字符串；否则返回格式化后的字符串
*/
template<class TChar, class... TParam>
std::basic_string<TChar> SafeFormat(const TChar *pFormat, TParam &&... args)
{
    try {
        boost::basic_format<TChar> formater(pFormat);
        detail::SafeFormatHelper(formater, std::forward<TParam>(args)...);
        return formater.str();
    } catch (const std::exception & /* err*/) {
        assert(!"format error");
        return std::basic_string<TChar>();
    }
}

/** 与SafeFormat相同，区别仅在于字符集转换，固定使用utf8-utf16进行宽窄字符转换
*/
template<class TChar, class... TParam>
std::basic_string<TChar> SafeFormatUtf8(const TChar *pFormat, TParam &&... args)
{
    try {
        /*  https://en.cppreference.com/w/cpp/locale/locale/locale
            Overload 7 is typically called with its second argument, f, 
        obtained directly from a new-expression: the locale is responsible
        for calling the matching delete from its own destructor.
        */
        boost::basic_format<TChar> formater(
            pFormat, std::locale{std::locale(), new std::codecvt_utf8_utf16<wchar_t>});
        detail::SafeFormatHelper(formater, std::forward<TParam>(args)...);
        return formater.str();
    } catch (const std::exception & /* err*/) {
        assert(!"format error");
        return std::basic_string<TChar>();
    }
}

SHARELIB_END_NAMESPACE
