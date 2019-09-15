#include <boost/config.hpp>
#if defined(BOOST_GCC)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined(BOOST_CLANG)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#include "Log/BoostLog.h"
#include <locale>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <boost/date_time.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/common.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#define BOOST_THREAD_USES_DATETIME
#include <boost/log/sinks.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/utility/setup.hpp>
#include "Log/SimpleAsyncSink.h"

#if defined(BOOST_GCC)
#    pragma GCC diagnostic pop
#elif defined(BOOST_CLANG)
#    pragma clang diagnostic pop
#endif

SHARELIB_BEGIN_NAMESPACE
namespace log {

static const char NAMED_SCOPE_ATTR[] = "NAMED_SCOPE_ATTR";

namespace default_names = boost::log::aux::default_attribute_names;

template<typename Stream>
void DefaultFormater(const boost::log::record_view &view, Stream &stream)
{
    stream << '['
           << boost::log::extract<boost::log::attributes::local_clock::value_type>(
                  default_names::timestamp(), view)
           << ']';

    auto &&channel = boost::log::extract<std::string>(default_names::channel(), view);
    if (channel)
    {
        stream << '[' << channel.get() << ']';
    }

    stream << '['
           << boost::log::extract<boost::log::attributes::current_thread_id::value_type>(
                  default_names::thread_id(), view)
           << ']';

    auto &&severityValue = boost::log::extract<LogLevel>(default_names::severity(), view);
    if (severityValue)
    {
        stream << '[' << ToString(severityValue.get()) << ']';
    }

    auto &&scopeValue = boost::log::extract<boost::log::attributes::named_scope::value_type>(
        NAMED_SCOPE_ATTR, view);
    if (scopeValue && !scopeValue.get_ptr()->empty())
    {
        stream << '['
               << boost::filesystem::path(scopeValue.get_ptr()->back().file_name.str())
                      .filename()
                      .string()
               << '(' << scopeValue.get_ptr()->back().line << "),"
               << scopeValue.get_ptr()->back().scope_name << ']';
    }

    stream << boost::log::extract(boost::log::expressions::smessage, view);
}

//---------------------------------------------------------------------

void InitLog()
{
    static std::once_flag s_init_flag;
    std::call_once(s_init_flag, []() {
        try
        {
            boost::log::core::get()->add_global_attribute(NAMED_SCOPE_ATTR,
                                                          boost::log::attributes::named_scope());
            boost::log::add_common_attributes();

            auto spTimeFacet =
                std::make_unique<boost::date_time::time_facet<boost::posix_time::ptime, char>>();
            spTimeFacet->set_iso_extended_format();
            auto spTimeFacetW =
                std::make_unique<boost::date_time::time_facet<boost::posix_time::ptime, wchar_t>>();
            spTimeFacetW->set_iso_extended_format();
            auto spDateFacet =
                std::make_unique<boost::date_time::date_facet<boost::gregorian::date, char>>();
            spDateFacet->set_iso_extended_format();
            auto spDateFacetW =
                std::make_unique<boost::date_time::date_facet<boost::gregorian::date, wchar_t>>();
            spDateFacetW->set_iso_extended_format();
            auto customLocal = std::locale(std::locale(), spTimeFacet.release());
            customLocal = std::locale(customLocal, spTimeFacetW.release());
            customLocal = std::locale(customLocal, spDateFacet.release());
            customLocal = std::locale(customLocal, spDateFacetW.release());
            std::locale::global(customLocal);
        }
        catch (const std::exception &err)
        {
            printf("%s\n", err.what());
        }
    });
}

void RemoveAllLogs()
{
    boost::log::core::get()->flush();
    boost::log::core::get()->remove_all_sinks();
}

void SetEnableLog(bool is_enable)
{
    boost::log::core::get()->set_logging_enabled(is_enable);
}

const char *ToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::debug:
        return "debug";
    case LogLevel::info:
        return "info";
    case LogLevel::warning:
        return "warning";
    case LogLevel::error:
        return "error";
    case LogLevel::fatal:
        return "fatal";
    default:
        return "";
    }
}

void SetGlobalLogLevel(LogLevel level)
{
    boost::log::core::get()->set_filter(
        [level](const boost::log::attribute_value_set &attrs) -> bool {
            auto &&value = attrs[default_names::severity()];
            if (value)
                return value.extract<LogLevel>() >= level;
            return true;
        });
}

boost::shared_ptr<boost::log::sinks::sink> AddFileLog(const FileLogParam &param)
{
    if (param.full_path_.empty())
    {
        return nullptr;
    }
    using TSinkBackend = boost::log::sinks::text_file_backend;
    auto ratotion_name = param.full_path_;
    {
        // 日志回滚
        static const char *ROTATION_PATTERN = "_%Y%m%d_%H%M%S";
        if (param.full_path_.has_extension())
        {
            auto file_name = param.full_path_.filename().string();
            auto extension = param.full_path_.extension().string();
            auto index = file_name.rfind(extension);
            file_name.erase(index);
            file_name += ROTATION_PATTERN + extension;
            ratotion_name.remove_filename();
            ratotion_name.append(file_name);
        }
        else
        {
            ratotion_name.append(ROTATION_PATTERN);
        }
    }

    boost::shared_ptr<TSinkBackend> sp_backend{new TSinkBackend(
        boost::log::keywords::file_name = param.full_path_,
        boost::log::keywords::target_file_name = ratotion_name,
        boost::log::keywords::open_mode = std::ios::out | std::ios::app | std::ios::ate,
        boost::log::keywords::rotation_size = param.rotation_size_,
        boost::log::keywords::enable_final_rotation = false,
        boost::log::keywords::auto_flush = (param.is_async_ ? false : param.is_auto_flush_))};

    {
        // 日志收集
        auto log_dir = param.full_path_;
        log_dir.remove_filename();
        auto spFileCollector = boost::log::sinks::file::make_collector(
            boost::log::keywords::target = log_dir,
            boost::log::keywords::max_size = param.max_backup_size_);
        sp_backend->set_file_collector(spFileCollector);
        sp_backend->scan_for_files();
    }

    boost::shared_ptr<boost::log::sinks::sink> sp_sink;
    {
        // filter
        auto filter = param.channel_filter_;
        auto level = param.level_;
        auto &&channel_filter = [filter,
                                 level](const boost::log::attribute_value_set &attrs) -> bool {
            auto &&severity = attrs[default_names::severity()];
            assert(severity);
            if (severity)
            {
                if (severity.extract<LogLevel>() >= level)
                {
                    if (filter.empty())
                    {
                        return true;
                    }
                    else
                    {
                        auto &&channel = attrs[default_names::channel()];
                        assert(channel);
                        if (channel)
                        {
                            auto &&channel_value = channel.extract<std::string>();
                            return channel_value == filter;
                        }
                    }
                }
            }
            return false;
        };

        // frontend
        if (param.is_async_)
        {
            using TSinkFrontend =
                SimpleAsyncSink<TSinkBackend,
                                boost::log::sinks::bounded_fifo_queue<
                                    500000,
                                    boost::log::sinks::drop_on_overflow // 日志最大数量，超出丢掉
                                    >>;
            auto sp_frontend = boost::make_shared<TSinkFrontend>(sp_backend);
            sp_sink = sp_frontend;
            sp_frontend->set_formatter(&DefaultFormater<TSinkFrontend::stream_type>);
            sp_frontend->set_filter(channel_filter);
        }
        else
        {
            using TSinkFrontend = boost::log::sinks::synchronous_sink<TSinkBackend>;
            auto sp_frontend = boost::make_shared<TSinkFrontend>(sp_backend);
            sp_sink = sp_frontend;
            sp_frontend->set_formatter(&DefaultFormater<TSinkFrontend::stream_type>);
            sp_frontend->set_filter(channel_filter);
        }
    }

    if (sp_sink)
        boost::log::core::get()->add_sink(sp_sink);

    return sp_sink;
}

void RemoveFileLog(boost::shared_ptr<boost::log::sinks::sink> sp_log)
{
    boost::log::core::get()->remove_sink(sp_log);
}

} // namespace log
SHARELIB_END_NAMESPACE
