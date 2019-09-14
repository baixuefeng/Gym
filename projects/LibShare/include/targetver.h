#pragma once

// 包括 SDKDDKVer.h 将定义可用的最高版本的 Windows 平台。

// 如果要为以前的 Windows 平台生成应用程序，请包括 WinSDKVer.h，并将
// WIN32_WINNT 宏设置为要支持的平台，然后再包括 SDKDDKVer.h。
#include <WinSDKVer.h>
#ifndef _WIN32_WINNT
//#define _WIN32_WINNT _WIN32_WINNT_WINXP
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#endif

//从 Windows 头文件中排除极少使用的信息，还可以避免windows.h放在Winsock2.h之前时产生的编译错误
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <SDKDDKVer.h>

