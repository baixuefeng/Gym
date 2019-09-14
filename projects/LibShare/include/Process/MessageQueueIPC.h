/******************************************************************************
*  版权所有（C）2018-2019，上海二三四五网络科技有限公司                       *
*  保留所有权利。                                                             *
*******************************************************************************
*  作者 : 白雪峰
*  版本 : 1.0
******************************************************************************/
#pragma once

#ifdef _WIN32

/* windows上默认是使用自旋锁实现的进程间等待，这会无谓地增加CPU context switch、CPU占用率，
因此取消该宏的定义，改用命名Semaphore实现的进程间等待。
*/
#include <boost/interprocess/detail/workaround.hpp>
#undef BOOST_INTERPROCESS_FORCE_GENERIC_EMULATION

/*
windows中ipc默认使用EventLog获取bootup time，基于此生成ipc的共享文件夹，但某些系统中可能获取失败。
比如可能被某些清理软件清除掉了EventLog，也可能EventLog满了自动清除了。因此这里统一为在程序路径下自建
一个ipc_share文件夹，作为ipc的共享文件夹。
bug参考: https://svn.boost.org/trac10/ticket/12137#no1
文档参考: boost_1_69_0/doc/html/interprocess/acknowledgements_notes.html

linux中是基于shared memory objects(shm_open)实现，不会调用该函数。
*/
#define BOOST_INTERPROCESS_SHARED_DIR_FUNC
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/filesystem.hpp>
#include <boost/dll.hpp>
namespace boost
{
    namespace interprocess
    {
        namespace ipcdetail
        {
            inline void get_shared_dir(std::string &shared_dir)
            {
                auto dir = boost::dll::program_location().remove_filename() / "ipc_share";
                if (!boost::filesystem::exists(dir))
                {
                    boost::filesystem::create_directories(dir);
                }
                shared_dir = dir.string();
            }
        }
    }
}

#else

#include <boost/interprocess/ipc/message_queue.hpp>

#endif
