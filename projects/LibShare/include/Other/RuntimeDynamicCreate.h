#pragma once
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

//运行时动态创建对象，实质就是根据字符串，new 出相应的类对象。
//需要自己实现字符串和创建函数的映射关系，并且保存起来，以备动态创建时使用。
//注意：
//1.支持运行时动态创建的类需要有一个共同的基类，动态创建出来的是基类的指针。
//2.基类需要有public虚析构函数，否则当delete这个基类指针时可能导致资源泄漏。
//3.支持运行时动态创建的类需要有一个默认构造函数。

//创建对象的函数原型
using TRuntimeCreateFunction = void * (*)();
//注册函数的函数原型
using TRegisterClassFunction = void(*)(const wchar_t * pCreateKay, TRuntimeCreateFunction pfnCreateFunc);

//把这个宏放到需要运行时动态创建的类声明里面
//className: 类名称
//pCreateKey: 创建字符串
#define DECLARE_RUNTIME_DYNAMIC_CREATE(className, pCreateKey) \
public:\
    virtual const wchar_t * GetCreateKey() \
    { \
        return pCreateKey; \
    } \
    static const wchar_t * GetMyCreateKey() \
    { \
        return pCreateKey; \
    } \
    struct Register##className##Helper \
    { \
        Register##className##Helper(); \
    }; \
    static Register##className##Helper& InitDynimicCreateInfo() \
    { \
        return s_autoRegister##className; \
    } \
private: \
    static className* CreateObject() \
    { \
        return new className; \
    } \
    friend struct className::Register##className##Helper; \
    static Register##className##Helper s_autoRegister##className;

//把这个宏放到需要运行时动态创建的类实现文件中
//className :类名称
//RegisterFunc :真实的注册函数,TRegisterClassFunction,会在main函数之前调用注册类信息
#define IMPLEMENT_RUNTIME_DYNAMIC_CREATE(className, RegisterFunc) \
    className::Register##className##Helper::Register##className##Helper() \
    { \
        (RegisterFunc)(className::GetMyCreateKey(), (TRuntimeCreateFunction)&className::CreateObject); \
    } \
    className::Register##className##Helper className::s_autoRegister##className;

SHARELIB_END_NAMESPACE
