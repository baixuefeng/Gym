﻿// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"
#ifndef _CRT_SECURE_NO_WARNINGS
#    define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _SCL_SECURE_NO_WARNINGS
#    define _SCL_SECURE_NO_WARNINGS
#endif
#ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN //  从 Windows 头文件中排除极少使用的信息
#endif

// Windows 头文件:
#include <windows.h>

// C 运行时头文件
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <tchar.h>

// TODO:  在此处引用程序需要的其他头文件
namespace ShareLibTest {}
#define BEGIN_SHARELIBTEST_NAMESPACE namespace ShareLibTest {
#define END_SHARELIBTEST_NAMESPACE }
