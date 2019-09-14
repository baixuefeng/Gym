#pragma once

#include "MacroDefBase.h"
#include "Other/OstreamCodeConvert.h"
#include "std_streambuf_adaptor.h"
#include "FakeOstream.h"

#ifdef NO_SHARE_LOG
#define tcout shr::FakeOstream()
#define tcoutex shr::FakeOstream()
#else
#define tcout shr::ConsoleSafeOstream(false, 0)
#define tcoutex shr::ConsoleSafeOstream(true, __LINE__)
#endif

SHARELIB_BEGIN_NAMESPACE

//初始化控件台，没有的话分配一个
void InitConsole();

//控件台输出，用tcout或tcoutex输出是线程安全的。必须有控制台才会输出。
class ConsoleSafeOstream
    : public std::wostream
{
public:
    explicit ConsoleSafeOstream(bool bTime = false, int nLine = 0);
    virtual ~ConsoleSafeOstream() override;

    ConsoleSafeOstream & Printf(const wchar_t * pFmt, ...);
private:
    wfix_buf m_buf;
};

SHARELIB_END_NAMESPACE
