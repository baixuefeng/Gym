#pragma once

#include "MacroDefBase.h"
#include "Other/OstreamCodeConvert.h"
#include "std_streambuf_adaptor.h"
#include "FakeOstream.h"

#ifdef NO_SHARE_LOG
#define logout(...) shr::FakeOstream()
#define logoutex(...) shr::FakeOstream()
#else
#define logout(pLog) shr::FileLogOstream(pLog, 1024, false, 0)
#define logoutex(pLog) shr::FileLogOstream(pLog, 1024, true, __LINE__)
#endif

SHARELIB_BEGIN_NAMESPACE

/** 设置全局Log的进程属性
@param[in] bOneLogPerProcess 每个进程单独一个日志文件
*/
void SetGlobalOneLogPerProcess(bool bOneLogPerProcess);

class FileLog;
class FileLogOstream
    : public std::ostream
{
public:
    explicit FileLogOstream(
        FileLog * pLog, //空值则使用全局日志
        size_t bufLen = 1024, //日志缓冲区大小
        bool bHhighResolution = false, //true表示高精度计时器
        int nLine = 0);//行号
    virtual ~FileLogOstream() override;

    FileLogOstream & Printf(const char * pFmt, ...);
private:
    fix_buf m_buf;
    FileLog * m_pLog;
};

//---------------------------------------------------------------------------

//多线程安全的文件日志
class FileLog
{
    SHARELIB_DISABLE_COPY_CLASS(FileLog);
public:
    /** 构造函数
    @param [in] pPath 日志路径，如果为nullptr或长度为0则取当前可执行文件所在文件夹
    @param [in] pName 日志名字，如果为nullptr或长度为0则取当前可执行文件的名字
    @param [in] bOneLogPerProcess 是否每个进程一个日志文件
    */
    explicit FileLog(
        const char *pPath = nullptr, 
        const char *pName = nullptr,
        bool bOneLogPerProcess = false);

    ~FileLog();

    /** 删除日志
    @param [in] bDeleteAll true表示删除所有的日志，false表示今天的不删除，其他的删除
    */
    void DeleteLogFile(bool bDeleteAll);

    /** 打开日志所在的文件夹
    */
    void ShowLogDir();

    /** 打开日志文件
    */
    void ShowLogFile();

private:
    friend class FileLogOstream;

    struct TImpl;
    TImpl * m_pImpl;
};

SHARELIB_END_NAMESPACE
