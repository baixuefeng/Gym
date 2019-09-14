#include "stdafx.h"
//#include <atomic>
//#include <clocale>
//#include <codecvt>
//#include <ciso646>
//#include <functional>
//#include <locale>
#include <memory>
//#include <math.h>
//#include <sstream>
//#include <thread>
#include <type_traits>
//#include <tuple>
#include <forward_list>
#include <thread>
//#include <vector>
//#include <valarray>
//#include <iterator>
//#include <shared_mutex>
#include <algorithm>
//#include <iostream>
//#include <fstream>
//#include <sstream>
//#include <chrono>
//#include <array>
//#include <unordered_map>

#include <boost/dll.hpp>
//#include <boost/test/minimal.hpp>
//#include <boost/preprocessor.hpp>
//#include <boost/preprocessor/repetition/repeat.hpp>
//#define BOOST_ASIO_DISABLE_STD_CHRONO
//#include <boost/process.hpp>
//#include <boost/asio.hpp>
//#include <boost/chrono.hpp>
//#include <boost/interprocess/ipc/message_queue.hpp>
//#include <boost/process/async_pipe.hpp>
//#include <boost/bimap.hpp>
//#include <boost/date_time.hpp>
//#include <boost/tokenizer.hpp>
#include <boost/log/trivial.hpp>
//#include <boost/thread/shared_mutex.hpp>
//#include <boost/lexical_cast.hpp>
//#include <boost/iterator.hpp>
//#include <boost/iterator/iterator_facade.hpp>
//#include <boost/tokenizer.hpp>
//#include <boost/utility/string_view.hpp>
//#include <boost/thread.hpp>
#include <boost/asio.hpp>
//#include <boost/stacktrace.hpp>
#include <boost/filesystem.hpp>
#include <boost/timer/timer.hpp>
//#include <boost/mp11.hpp>
//#include <boost/callable_traits.hpp>
//#include <atlbase.h>
//#include <atlcom.h>
//#include <atlmem.h>
//#include <atlwin.h>
//#include <atlstdthunk.h>
#include <boost/asio/ssl.hpp>

//#define  NO_SHARE_LOG
#include "Log/BoostLog.h"
#include "Log/TempLog.h"
//#include "Log/FileLog.h"
//#include "TestUtility/TestOpenGL.h"
//#include "Memory/std_streambuf_adaptor.h"
//#include "Memory/SmartWinHandle.h"
//#include "Memory/std_allocator_adaptor.h"
//#include "Other/VersionTime.h"
//#include "Other/PerformanceTest.h"
//#include "Other/TraversalTree.h"
//#include "TemplateMeta/MetaUtility.h"
//#include "IOCP/IocpThreadPool.h"
//#include "Thread/lockfree_queue.h"
//#include "Thread/thread_lock.h"
//#include "Config/JsonUtility.h"
//#include "minhook/include/MinHook.h"
#include "Other/SafeFormat.h"
//#include "Other/Base64Iter.h"
//#include "UI/Utility/WindowUtility.h"
//#include "UI/GraphicLayer/Layers/GraphicLayer.h"

//#include "../DllTest/classes/ITestCom1.h"

#include "ShareLibTest.h"
//#include "TestUnit/MatrixTest.h"
//#include "TestUnit/ReadWriterLockTest.h"
//#include "UI/D3D/D3DMisc.h"
//#include "TestUnit/WindowTest.h"
//#include "TestUnit/WebCurlTest.h"
//#include "TestUnit/LuaCppTest.h"
//#include "TestUnit/TestParallelQueue.h"
//#include "TestUnit/OpenGLTest.h"
//#include <openssl/ssl.h>
using namespace ShareLibTest;
//using namespace std;

#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using boost::asio::ip::tcp;
using std::placeholders::_1;
using std::placeholders::_2;

enum
{
    max_length = 4096
};

class client
{
public:
    client(boost::asio::io_context &io_context,
           boost::asio::ssl::context &context,
           const tcp::endpoint &endpoints) :
        socket_(io_context, context)
    {
        socket_.set_verify_mode(boost::asio::ssl::verify_peer |
                                boost::asio::ssl::verify_fail_if_no_peer_cert);
        socket_.set_verify_callback(std::bind(&client::verify_certificate, this, _1, _2));

        connect(endpoints);
    }

private:
    bool verify_certificate(bool preverified, boost::asio::ssl::verify_context &ctx)
    {
        // The verify callback can be used to check whether the certificate that is
        // being presented is valid for the peer. For example, RFC 2818 describes
        // the steps involved in doing this for HTTPS. Consult the OpenSSL
        // documentation for more details. Note that the callback is called once
        // for each certificate in the certificate chain, starting from the root
        // certificate authority.

        // In this example we will simply print the certificate's subject name.
        auto err = X509_STORE_CTX_get_error(ctx.native_handle());
        BOOST_LOG_TRIVIAL(info) << err << "  " << X509_verify_cert_error_string(err);

        X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        auto pBio = BIO_new(BIO_s_mem());
        X509_print_ex(pBio, cert, 0, XN_FLAG_RFC2253);
        auto length = BIO_ctrl_pending(pBio);
        std::string ss;
        ss.resize(length);
        BIO_read(pBio, &ss[0], ss.size());
        BOOST_LOG_TRIVIAL(info) << ss;

        auto pName = X509_get_subject_name(cert);
        X509_NAME_print_ex(pBio, pName, 0, XN_FLAG_RFC2253);
        length = BIO_ctrl_pending(pBio);
        ss.resize(length);
        BIO_read(pBio, &ss[0], ss.size());
        BOOST_LOG_TRIVIAL(info) << ss;
        BIO_free(pBio);

        return preverified;
    }

    void connect(const tcp::endpoint &endpoints)
    {
        socket_.lowest_layer().async_connect(
            endpoints, [this](const boost::system::error_code &error) {
                if (!error)
                {
                    handshake();
                }
                else
                {
                    std::cout << "Connect failed: " << error.message() << "\n";
                }
            });
    }

    void handshake()
    {
        socket_.async_handshake(boost::asio::ssl::stream_base::client,
                                [this](const boost::system::error_code &error) {
                                    if (!error)
                                    {
                                        send_request();
                                    }
                                    else
                                    {
                                        std::cout << "Handshake failed: " << error.message()
                                                  << "\n";
                                    }
                                });
    }

    void send_request()
    {
        std::string msg;
        msg.assign(1024, '0');
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(msg),
                                 [this](const boost::system::error_code &error,
                                        std::size_t length) {
                                     if (!error)
                                     {
                                         receive_response(length);
                                     }
                                     else
                                     {
                                         std::cout << "Write failed: " << error.message() << "\n";
                                     }
                                 });

        //std::cout << "Enter message: ";
        //std::cin.getline(request_, max_length);
        //size_t request_length = std::strlen(request_);

        //boost::asio::async_write(socket_,
        //    boost::asio::buffer(request_, request_length),
        //    [this](const boost::system::error_code& error, std::size_t length)
        //{
        //    if (!error)
        //    {
        //        receive_response(length);
        //    }
        //    else
        //    {
        //        std::cout << "Write failed: " << error.message() << "\n";
        //    }
        //});
    }

    void receive_response(std::size_t length)
    {
        boost::asio::async_read(socket_,
                                boost::asio::buffer(reply_, length),
                                [this](const boost::system::error_code &error, std::size_t length) {
                                    if (!error)
                                    {
                                        std::cout << "Reply: ";
                                        std::cout.write(reply_, length);
                                        std::cout << "\n";
                                        ::Sleep(2000);
                                        send_request();
                                    }
                                    else
                                    {
                                        std::cout << "Read failed: " << error.message() << "\n";
                                    }
                                });
    }

    boost::asio::ssl::stream<tcp::socket> socket_;
    char request_[max_length];
    char reply_[max_length];
};

static void DoAccept(boost::asio::ip::tcp::acceptor &server,
                     const boost::system::error_code &error,
                     boost::asio::ip::tcp::socket &&socket)
{
    server.async_accept(
        [&server](const boost::system::error_code &error, boost::asio::ip::tcp::socket &&socket) {
            DoAccept(server, error, std::move(socket));
        });

    if (!error)
    {
        BOOST_LOG_TRIVIAL(info) << socket.remote_endpoint();
    }
    else
    {
        BOOST_LOG_FUNC()
        BOOST_LOG_TRIVIAL(error) << error.message();
    }
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                       _In_opt_ HINSTANCE hPrevInstance,
                       _In_ LPTSTR lpCmdLine,
                       _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    shr::InitConsole();
    shr::log::InitLog();

    try
    {
        boost::asio::io_context io_context;

        boost::asio::ssl::context context_(boost::asio::ssl::context::tlsv13);
        context_.set_password_callback(
            [](std::size_t max_length,
               boost::asio::ssl::context::password_purpose purpose) -> std::string {
                return "dingant";
            });

        context_.load_verify_file(R"(E:\dev_dir\linux_project\dingant\etc\ssl_key\cacert.pem)");
        context_.use_certificate_chain_file(
            R"(E:\dev_dir\linux_project\dingant\etc\ssl_key\usercert.pem)");
        context_.use_private_key_file(R"(E:\dev_dir\linux_project\dingant\etc\ssl_key\userkey.pem)",
                                      boost::asio::ssl::context::pem);
        //context_.use_tmp_dh_file(R"(E:\dev_dir\linux_project\test-key\client-dh.pem)");

        boost::asio::ip::tcp::endpoint addr{boost::asio::ip::make_address("127.0.0.1"), 1514};
        client c(io_context, context_, addr);

        io_context.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    getchar();
#if 0

    try
    {
        if (!*lpCmdLine)
        {
            shr::InitConsole();

            boost::process::async_pipe writePipe{ iodevice };
            std::string content;
            content.resize(1000);
            std::function<void(boost::system::error_code, std::size_t)> handler;
            handler = [&handler, &content, &writePipe](boost::system::error_code err, std::size_t length)
            {
                if (err)
                {
                    tcout << err.message() << std::endl;
                }
                else
                {
                    content.erase(length);
                    tcout << content << std::endl;
                    content.resize(1000);
                    writePipe.async_read_some(boost::asio::buffer(content), handler);
                }
            };

            boost::process::spawn(R"(F:\VS_Project\public\ShareLibTest\bin\Win32\MTd\ShareLibTest.exe  subprocess)", boost::process::std_out > writePipe);

            writePipe.async_read_some(boost::asio::buffer(content), handler);

            //writePipe.write_some(boost::asio::buffer("aaaaaaaaaa"));

            iodevice.get_executor().on_work_started();
            iodevice.run();
        }
        else
        {
            //::AllocConsole();

            //boost::process::async_pipe pipe{ iodevice, PIPE_NAME };
            //tcout << pipe.is_open() << std::endl;
            //if (pipe.is_open())
            //{
            //    pipe.write_some(boost::asio::buffer("hello"));
            //}
            printf("hello main process\n");
            fflush(stdout);

            //iodevice.get_executor().on_work_started();
            //iodevice.run();
        }
    }
    catch (const std::exception& err)
    {
        tcout << err.what() << std::endl;
    }



    ip::udp::socket ss{ iodevice };
    ss.open(ip::udp::v4());
    ss.bind(ip::udp::endpoint{ ip::make_address("0.0.0.0"), 6770 });

    ss.set_option(socket_base::broadcast{ true });
    ss.set_option(socket_base::reuse_address{ true });
    ss.set_option(ip::multicast::join_group{ ip::make_address("239.192.152.142") });


    std::string content;
    content.resize(1000);
    auto && buf = boost::asio::buffer(content);
    ip::udp::endpoint remoteAddr;
    std::function<void(const boost::system::error_code&, std::size_t)> handler;
    handler = [&content, &buf, &remoteAddr, &ss, &handler](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        if (error)
        {
            tcout << error.message() << "\n";
        }
        else
        {
            content.erase(bytes_transferred);
            tcout << "remote: " << remoteAddr << "[" << content << "]\n";
            content.resize(1000);
            buf = boost::asio::buffer(content);
            ss.async_receive_from(buf, remoteAddr, handler);
        }
    };
    ss.async_receive_from(buf, remoteAddr, handler);
    //ss.async_connect(ip::tcp::endpoint{ip::make_address("192.168.1.101"), 6666},
    //    [](boost::system::error_code err)
    //{
    //    tcout << err.value() << " " << err.message() << std::endl;
    //    if (!err)
    //    {
    //        //ss.send()
    //    }
    //});

#endif

    {
        //CompareMatrixCalculate();
    }

    {
        //TestReadLock();
        //TestWriteLock();
        //TestReadWriteLock();
        //TestPriority();
    }

    {
        //TestWebCurl();
    }

    {
        //std::locale::global(std::locale(""));
        //TestLuaCpp();
    }

    {
        //TestParallelQueue{}.Test();
        //TestParallelQueue{}.TestMsgQueue();
    }

    {
        //shr::UniqueDllModule dll{ L"DllTest.dll" };
        //if (dll)
        //{
        //    DllFunc pf = (DllFunc)::GetProcAddress(dll, "DllWriteLog");
        //    pf();
        //}
        //getchar();
    }

    {
        //TestNetDelay();
    }

    //{
    //    auto hDll = ::LoadLibrary(LR"(F:\vsproject\public\ShareLibTest\bin\Win32\MDd\DllTest.dll)");
    //    if (hDll)
    //    {
    //        TDllRegisterServer pfn = (TDllRegisterServer)::GetProcAddress(hDll, "DllRegisterServer");
    //        if (pfn)
    //        {
    //            (pfn)();
    //        }
    //    }
    //}

    //ATL::CComPtr<ICatInformation> spCatInfo;
    //HRESULT hr = spCatInfo.CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER);
    //if (SUCCEEDED(hr))
    //{
    //    CComPtr<IEnumGUID> spEnumGUID;
    //    hr = spCatInfo->EnumImplCategoriesOfClass(CLSID_TestCom1, &spEnumGUID);
    //    if (SUCCEEDED(hr))
    //    {
    //        GUID implGUIDs[10]{};
    //        ULONG ulCount = 0;
    //        hr = spEnumGUID->Next(10, implGUIDs, &ulCount);

    //        wchar_t szBuffer[50]{0};
    //        for (ULONG i = 0; i < ulCount; ++i)
    //        {
    //            ::StringFromGUID2(implGUIDs[i], szBuffer, _countof(szBuffer));
    //            tcout << szBuffer << L'\n';
    //        }
    //    }
    //}

    {
        //TestWindow mainWindow;
        //mainWindow.Create(NULL, 0, 0, WS_OVERLAPPEDWINDOW, 0/*WS_EX_LAYERED*/);
        //mainWindow.CenterWindow();
        //mainWindow.ShowWindow(nCmdShow);
        //WTL::CMessageLoop{}.Run();
    }
    return 0;
}
