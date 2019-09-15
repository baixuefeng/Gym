#pragma once

#include <cstdio>
#include <fstream>
#include <mutex>
#include <string>
#include <windows.h>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

const long MAX_LOG_FILE_SIZE = 50 * 1024 * 1024; //单个日志文件的最大长度

/*!
 * \class LogFileManager
 * \brief 此类不是线程安全的
 */
class LogFileManager
{
    SHARELIB_DISABLE_COPY_CLASS(LogFileManager);

public:
    LogFileManager();

    ~LogFileManager();

    /** 初始化日志路径及名称
    @param [in] pPath 日志路径，如果为nullptr或长度为0则取当前可执行文件所在文件夹
    @param [in] pName 日志名字，如果为nullptr或长度为0则取当前可执行文件的名字
    @param [in] bOneLogPerProcess 是否每个进程一个日志文件
    */
    bool SetLogPathAndName(const char *pPath, const char *pName, bool bOneLogPerProcess = false);

    /** 写入日志
    @param [in] uiCount 为0表示写入到'\0'结束，否则写入指定个数的字节
    */
    bool Write(const char *pkszMsg, size_t uiCount = 0);

    const char *GetErrMsg();

    void Flush();

    const std::string &GetLogFullPath() const;

    /** 删除日志
    @param [in] bDeleteAll true表示删除所有的日志，false表示今天的不删除，其他的删除
    */
    void DeleteLogFile(bool bDeleteAll);

private:
    bool m_bRunOK;
    std::FILE *m_pFile;
    std::string m_strFilePath; //日志文件存放文件夹
    std::string m_strFileName; //当前写入日志的具体文件名

    mutable std::mutex m_lockFileFullPath;
    std::string m_strFileFullName; //当前写入日志的文件的全路径名

    std::string m_strErrorMsg;
    bool m_bNeedFlush;
};

SHARELIB_END_NAMESPACE
