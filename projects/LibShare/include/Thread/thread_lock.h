#pragma once
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <boost/core/noncopyable.hpp>
#include "MacroDefBase.h"

//----------------------------------
// C++11的锁在main函数之外进行加锁解锁可能会崩溃、永久阻塞等问题

SHARELIB_BEGIN_NAMESPACE

/** 类似windows的Event的锁
*/
class EventLock :
    protected boost::noncopyable
{
public:
    /** 构造函数
    @param[in] bManualReset 是否手动重置，如果为是，wait成功之后必须手工调用ResetEvent
    @param[in] bInitialState 初始状态是否激活
    */
    EventLock(bool bManualReset, bool bInitialState);

    /** 激活事件，使其它等待的线程可以退出等待
    */
    void SetEvent();

    /** 重置事件到非激活状态
    */
    void ResetEvent();

    /** 等待事件被激活
    @param[in] nMilliseconds 0表示不等待，<0表示永久等待，>0表示最多等待指定的时间(毫秒)
    @return 返回true表示成功,false表示超时
    */
    bool Wait(int64_t nMilliseconds = -1);

private:
    //是否手动重置
    const bool m_bManualReset;

    //是否被激活
    bool m_bActive;

    //线程锁
    std::mutex m_lock;

    //条件变量
    std::condition_variable m_condition;
};

//----------------------------------------------------------------------

/** 轻量级的线程同步锁:信号量
*/
class Semaphore :
    protected boost::noncopyable
{
public:
    /** 构造函数
    @param[in] nInitCount 初始个数
    */
	explicit Semaphore(size_t nInitCount);

    /** 请求资源, 请求多个的时候要么全部获得, 要么一个也不会获得
    @param[in] nCount 请求的资源数量
    @param[in] nMilliseconds 0表示不等待，<0表示永久等待
    */
	bool Acquire(size_t nCount, int64_t nMilliseconds = -1);

    /** 释放资源
    @param[in] nCount 释放的资源个数
    @param[in] bNotifyAll 是否激活所有等待的线程，false表示只激活一个
    @return 释放后当前的资源个数
    */
    size_t Release(size_t nCount, bool bNotifyAll = true);

private:
    //资源个数
    size_t m_count;

    //线程锁
    std::mutex m_lock;

    //条件变量
    std::condition_variable m_condition;
};

SHARELIB_END_NAMESPACE
