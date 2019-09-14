#pragma once
#include <boost/atomic.hpp>
#include <boost/log/sinks/basic_sink_frontend.hpp>
#include <boost/log/sinks/frontend_requirements.hpp>
#include <boost/log/sinks/unbounded_fifo_queue.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

/** 复制自 boost::log::sinks::asynchronous_sink，主要为了解决以下问题：
 *  text_file_backend 可以设置是否自动flush，为true时每输出一条日志就flush，可以及时落盘但速度慢，
 *  false时速度快但如果不手动调flush就不落盘，不能及时看到日志。速度和即时性不能很好兼顾。这里的实现
 *  修改为，当队列中有数据时不flush，队列为空时flush。text_file_backend设置为不自动flush，这样就
 *  可以实现高负载时速度优先，低负载时落盘，两方兼顾。
 *
 *  其他区别：
 *  去掉了boost::log::sinks::asynchronous_sink中的flush相关的public接口，因为内部实现已经兼顾
 *  速度和即时性，没有必要再留这些接口了。
 */
template<typename SinkBackendT,
         typename QueueingStrategyT = boost::log::sinks::unbounded_fifo_queue>
class SimpleAsyncSink :
    public boost::log::sinks::aux::make_sink_frontend_base<SinkBackendT>::type,
    public QueueingStrategyT
{
    typedef typename boost::log::sinks::aux::make_sink_frontend_base<SinkBackendT>::type base_type;
    typedef QueueingStrategyT queue_base_type;

    //! Backend synchronization mutex type
    typedef boost::recursive_mutex backend_mutex_type;
    //! Frontend synchronization mutex type
    typedef typename base_type::mutex_type frontend_mutex_type;

public:
    //! Sink implementation type
    typedef SinkBackendT sink_backend_type;
    //! \cond
    BOOST_STATIC_ASSERT_MSG(
        (boost::log::sinks::has_requirement<typename sink_backend_type::frontend_requirements,
                                            boost::log::sinks::synchronized_feeding>::value),
        "Asynchronous sink frontend is incompatible with the specified backend: thread "
        "synchronization requirements are not met");
    //! \endcond

    /*!
    * Constructor attaches user-constructed backend instance
    *
    * \param backend Pointer to the backend instance.
    *
    * \pre \a backend is not \c NULL.
    */
    explicit SimpleAsyncSink(boost::shared_ptr<sink_backend_type> const &backend) :
        base_type(true), m_pBackend(backend), m_StopRequested(false)
    {
        boost::thread(boost::bind(&SimpleAsyncSink::run, this)).swap(m_DedicatedFeedingThread);
    }

    /*!
    * Destructor. Implicitly stops the dedicated feeding thread, if one is running.
    */
    ~SimpleAsyncSink() BOOST_NOEXCEPT
    {
        try
        {
            boost::this_thread::disable_interruption no_interrupts;
            stop();
        }
        catch (...)
        {
            std::terminate();
        }
    }

    /*!
    * Enqueues the log record to the backend
    */
    void consume(boost::log::record_view const &rec) { queue_base_type::enqueue(rec); }

    /*!
    * The method attempts to pass logging record to the backend
    */
    bool try_consume(boost::log::record_view const &rec)
    {
        return queue_base_type::try_enqueue(rec);
    }

    void flush() {}

private:
    /*!
    * The method starts record feeding loop and effectively blocks until either of this happens:
    *
    * \li the thread is interrupted due to either standard thread interruption or a call to \c stop
    * \li an exception is thrown while processing a log record in the backend, and the exception is
    *     not terminated by the exception handler, if one is installed
    *
    * \pre The sink frontend must be constructed without spawning a dedicated thread
    */
    void run()
    {
        for (;;)
        {
            do_feed_records();
            if (!m_StopRequested.load(boost::memory_order_acquire))
            {
                // Block until new record is available
                boost::log::record_view rec;
                if (queue_base_type::dequeue_ready(rec))
                    base_type::feed_record(rec, m_BackendMutex, *m_pBackend);
            }
            else
                break;
        }
    }

    //! The record feeding loop
    void do_feed_records()
    {
        while (!m_StopRequested.load(boost::memory_order_acquire))
        {
            boost::log::record_view rec;
            if (queue_base_type::try_dequeue_ready(rec))
                base_type::feed_record(rec, m_BackendMutex, *m_pBackend);
            else
                break;
        }
        base_type::flush_backend(m_BackendMutex, *m_pBackend);
    }

    /*!
    * The method softly interrupts record feeding loop. This method must be called when the \c run
    * method execution has to be interrupted. Unlike regular thread interruption, calling
    * \c stop will not interrupt the record processing in the middle. Instead, the sink frontend
    * will attempt to finish its business with the record in progress and return afterwards.
    * This method can be called either if the sink was created with a dedicated thread,
    * or if the feeding loop was initiated by user.
    *
    * \note Returning from this method does not guarantee that there are no records left buffered
    *       in the sink frontend. It is possible that log records keep coming during and after this
    *       method is called. At some point of execution of this method log records stop being processed,
    *       and all records that come after this point are put into the queue. These records will be
    *       processed upon further calls to \c run or \c feed_records.
    */
    void stop()
    {
        boost::unique_lock<frontend_mutex_type> lock(base_type::frontend_mutex());
        if (m_DedicatedFeedingThread.joinable())
        {
            try
            {
                m_StopRequested.store(true, boost::memory_order_release);
                queue_base_type::interrupt_dequeue();
            }
            catch (...)
            {
                m_StopRequested.store(false, boost::memory_order_release);
                throw;
            }

            lock.unlock();
            m_DedicatedFeedingThread.join();
        }
    }

private:
    //! Synchronization mutex
    backend_mutex_type m_BackendMutex;
    //! Pointer to the backend
    const boost::shared_ptr<sink_backend_type> m_pBackend;

    //! Dedicated record feeding thread
    boost::thread m_DedicatedFeedingThread;
    //! Condition variable to implement blocking operations
    boost::condition_variable_any m_BlockCond;

    //! The flag indicates that the feeding loop has to be stopped
    boost::atomic<bool> m_StopRequested;
};

SHARELIB_END_NAMESPACE
