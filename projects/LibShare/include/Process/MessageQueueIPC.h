/******************************************************************************
*  ��Ȩ���У�C��2018-2019���Ϻ�������������Ƽ����޹�˾                       *
*  ��������Ȩ����                                                             *
*******************************************************************************
*  ���� : ��ѩ��
*  �汾 : 1.0
******************************************************************************/
#pragma once

#ifdef _WIN32

/* windows��Ĭ����ʹ��������ʵ�ֵĽ��̼�ȴ��������ν������CPU context switch��CPUռ���ʣ�
���ȡ���ú�Ķ��壬��������Semaphoreʵ�ֵĽ��̼�ȴ���
*/
#include <boost/interprocess/detail/workaround.hpp>
#undef BOOST_INTERPROCESS_FORCE_GENERIC_EMULATION

/*
windows��ipcĬ��ʹ��EventLog��ȡbootup time�����ڴ�����ipc�Ĺ����ļ��У���ĳЩϵͳ�п��ܻ�ȡʧ�ܡ�
������ܱ�ĳЩ��������������EventLog��Ҳ����EventLog�����Զ�����ˡ��������ͳһΪ�ڳ���·�����Խ�
һ��ipc_share�ļ��У���Ϊipc�Ĺ����ļ��С�
bug�ο�: https://svn.boost.org/trac10/ticket/12137#no1
�ĵ��ο�: boost_1_69_0/doc/html/interprocess/acknowledgements_notes.html

linux���ǻ���shared memory objects(shm_open)ʵ�֣�������øú�����
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
