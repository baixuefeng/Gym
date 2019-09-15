#pragma once

#if defined(_WIN32) && defined(_MSC_VER)

//自动链接静态库
#    ifdef _M_IX86
#        define SHARELIB_PLATFORM_CONFIG "_x86"
#    else
#        define SHARELIB_PLATFORM_CONFIG "_x64"
#    endif

//_DLL: Defined when /MD or /MDd (Multithreaded DLL) is specified.
#    ifdef _DLL
#        define SHARELIB_RUNTIMELIB_CONFIG "_MD"
#    else
#        define SHARELIB_RUNTIMELIB_CONFIG "_MT"
#    endif

#    ifdef _DEBUG
#        define SHARELIB_RELEASE_CONFIG "d"
#    else
#        define SHARELIB_RELEASE_CONFIG
#    endif

#    if !defined(SHARELIB_NO_AUTO_LINK) && (defined(_CONSOLE) || !defined(_LIB))
//#pragma comment(lib, "libShare" SHARELIB_PLATFORM_CONFIG SHARELIB_RUNTIMELIB_CONFIG SHARELIB_RELEASE_CONFIG ".lib")
#    endif
#    undef SHARELIB_PLATFORM_CONFIG
#    undef SHARELIB_RUNTIMELIB_CONFIG
#    undef SHARELIB_RELEASE_CONFIG

/* 一些有用的宏
_LIB     工程为静态库或控件台程序时会定义该宏
_WINDLL  工程为动态库时会定义该宏
*/

#endif

//命名空间
#ifndef SHARELIB_BEGIN_NAMESPACE
#    define SHARELIB_BEGIN_NAMESPACE namespace shr {
#endif
#ifndef SHARELIB_END_NAMESPACE
#    define SHARELIB_END_NAMESPACE }
#endif

//禁用复制
#ifndef SHARELIB_DISABLE_COPY_CLASS
#    define SHARELIB_DISABLE_COPY_CLASS(className)                                                 \
        className(const className &) = delete;                                                     \
        className &operator=(const className &) = delete
#endif

/* 注释规则
1. 大的分割块用 //----注释内容--------, 不缩进,顶头
2. 函数注释如果很简单，单行就能说明，用双斜线；超过单行用 doxygen的简易模式：
   普通变量用 @param[in,opt,out],模块参数用 @Tparam，返回值用@return
3. 单行注释用双斜线, 多行注释用单斜线加星呈. 多行注释, 第一行起头空一格, 后写简短标题; 后面顶头写，每行开头不加星
4. 变量的注释不放到变量后面，enum的值，如果全部都比较简单，单行能写完，可以写后面，对齐
*/

/* 编码规则
1. 使用VS默认的规则
2. if-else之类，即使只有一行也使用｛｝
3. 函数参数过长，全部另抬一行，缩进，坚排对齐；每嵌套一层多缩进一级。
4. 函数及类，非跨平台的用微软方式，跨平台的用stl方式，局部变量用小驼峰方式。
5. 接口类以I开头，类型重定义及enum，union,struct以T开头，类无前缀, 多用命名空间
*/
