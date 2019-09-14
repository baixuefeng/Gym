#pragma once

#include <cstdint>
#include <boost/filesystem.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/sinks/sink.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE
namespace log{

/** 初始化日志系统。
*  多线程安全，可重入。
*/
void InitLog();

/** 设置日志系统是否开启，默认为开启状态。关闭后所有日志都禁用。
*  多线程安全，可重入。
* @param[in] is_enable 是否开启
*/
void SetEnableLog(bool is_enable);

/** 日志级别
*/
enum class LogLevel
{
    debug = 0,
    info,
    warning,
    error,
    fatal
};

/** 日志级别转化为字符串
*/
const char *ToString(LogLevel level);

/** 设置日志过滤级别，多线程安全，可以多次调用，并且可以在运行时调用。
*  默认输出所有日志
* @param[in] level 大于等于该级别的日志才会输出
*/
void SetGlobalLogLevel(LogLevel level);

/** 文件日志参数
*/
struct FileLogParam
{
    /// 日志存放的全路径
    boost::filesystem::path full_path_;

    /// 日志过滤器，如果非空，只有 channel 与此相同的日志才会输出到该文件
    std::string channel_filter_;

    /// 单个日志的过滤级别。需要先通过全局的日志级别过滤，才会检查单个日志的过滤条件。
    LogLevel level_ = LogLevel::debug;

    /// 是否使用异步日志，如果使用异步日志，每一个日志会开启一个后台线程
    bool is_async_ = true;

    /// 日志是否自动刷新，is_async_为true时该参数无效，会根据负载自动调节
    bool is_auto_flush_ = true;

    /// 日志回滚大小
    uintmax_t rotation_size_ = 10 * 1024 * 1024;

    /// 最大保存日志的大小
    uintmax_t max_backup_size_ = rotation_size_;
};

/** 添加文件日志
* @param param 日志参数
* @return 文件日志指针，注意，这个指针只有两个用途：
*      1. flush(is_auto_flush_为false时才有必要)
*      2. RemoveFileLog
*/
boost::shared_ptr<boost::log::sinks::sink> AddFileLog(const FileLogParam &param);

/** 删除文件日志，已经保存在磁盘上的日志文件不会删除
* @param sp_log 文件日志指针
*/
void RemoveFileLog(boost::shared_ptr<boost::log::sinks::sink> sp_log);

/** 删除所有日志
*/
void RemoveAllLogs();

/** 日志source单例定义
*/
BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(global_logger,
    boost::log::sources::severity_channel_logger_mt<LogLevel>)

} // namespace log
SHARELIB_END_NAMESPACE

/** 示例： SHARE_LOG("net_work", info) << "connect sucess!";
 */
#define SHARE_LOG(channel, level)                                                                  \
    BOOST_LOG_CHANNEL_SEV(                                                                         \
        ::shr::log::global_logger::get(), channel, ::shr::log::LogLevel::level)
