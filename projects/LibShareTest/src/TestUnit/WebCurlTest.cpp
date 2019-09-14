#include "stdafx.h"
#include "TestUnit/WebCurlTest.h"
#include <cstdio>
#include <future>
#include "web/LibcurlWrapper.h"
#include "log/TempLog.h"

BEGIN_SHARELIBTEST_NAMESPACE

#pragma warning(disable:4996)

void TestWebCurl()
{
    shr::InitGlobalLibcurl();
    std::FILE* pFile = std::fopen("WebWrite.log", "wb");
    std::FILE* pFileDebug = std::fopen("WebDebug.log", "wb");

    shr::CurlMultiHandle multiHandle;
    shr::CurlEasyHandle easyHandle;
    multiHandle.AddEasyHandle(easyHandle);
    easyHandle.SetUrl("http://101.44.1.124/files/A105000006DD021F/dl.2345.com/haozip/haozip_v5.9.5.exe");
    easyHandle.SetFollowLocation(true);
    //easyHandle.AddHttpHeaders({ "Hey-server-hey: how are you?" });
    //easyHandle.AddHttpHeaders({ "ABCD: 1234" });
    //easyHandle.SetPostFields("name=daniel&project=curl");
    //easyHandle.SetHeaderBody(true, true);
    easyHandle.SetSSLVerify(false, false);


    easyHandle.SetHeaderCallback(
        [](const char * pBuffer, size_t nBytes)
    {
        (void)nBytes;
        tcout << L"length=" << nBytes << L"content=" << pBuffer;
    });
    easyHandle.SetWriteCallback(
        [pFile](const char * pBuffer, size_t nBytes)
    {
        std::fwrite(pBuffer, nBytes, 1, pFile);
    });
    easyHandle.SetProgressCallback(
        [](int64_t nDownloadTotal,
        int64_t nDownloadNow,
        int64_t nUploadTotal,
        int64_t nUploadNow)->bool
    {
        if (nDownloadTotal > 0)
        {
            tcout << "download: " << nDownloadNow << " in " << nDownloadTotal  << std::endl;
        }
        if (nUploadTotal > 0)
        {
            tcout << "upload: " << nUploadNow << " in " << nUploadTotal << std::endl;
        }
        return true;
    });

    easyHandle.SetDebugCallback(
        [pFileDebug](curl_infotype type, const char * pBuffer, size_t nSize)
    {
        std::string typeInfoStr;
        switch (type)
        {
        case CURLINFO_TEXT:
            typeInfoStr = "[DebugInfo:text]";
            break;
        case CURLINFO_HEADER_IN:
            typeInfoStr = "[DebugInfo:HeaderIn]";
            break;
        case CURLINFO_HEADER_OUT:
            typeInfoStr = "[DebugInfo:HeaderOut]";
            break;
        case CURLINFO_DATA_IN:
            typeInfoStr = "[DebugInfo:DataIn]";
            nSize = 0;
            break;
        case CURLINFO_DATA_OUT:
            typeInfoStr = "[DebugInfo:DataOut]";
            nSize = 0;
            break;
        case CURLINFO_SSL_DATA_IN:
            typeInfoStr = "[DebugInfo:SslDataIn]";
            nSize = 0;
            break;
        case CURLINFO_SSL_DATA_OUT:
            typeInfoStr = "[DebugInfo:SslDataOut]";
            nSize = 0;
            break;
        default:
            break;
        }
        std::fwrite(typeInfoStr.c_str(), typeInfoStr.size(), 1, pFileDebug);
        std::fwrite(pBuffer, nSize, 1, pFileDebug);
        std::fwrite("\n", 1, 1, pFileDebug);
    });

    auto res = std::async(
        [&multiHandle]()
    {
        return multiHandle.Perform(0);
    });
    if (IDYES == ::MessageBox(NULL, L"是否中止?", L"提示", MB_YESNO))
    {
        multiHandle.Terminate();
    }
    tcout << "multi perform=" << res.get() << std::endl;
    //auto msgs = multiHandle.GetInfoRead();

    tcout << easyHandle.GetInfo<CURLINFO::CURLINFO_PRIMARY_IP>() << L" "
        << easyHandle.GetInfo<CURLINFO::CURLINFO_PRIMARY_PORT>() << std::endl;
    tcout << easyHandle.GetInfo<CURLINFO::CURLINFO_RESPONSE_CODE>() << std::endl;
    tcout << easyHandle.GetInfo<CURLINFO::CURLINFO_HEADER_SIZE>() << std::endl;
    tcout << easyHandle.GetInfo<CURLINFO::CURLINFO_CONTENT_TYPE>() << std::endl;
    tcout << easyHandle.GetInfo<CURLINFO::CURLINFO_CONTENT_LENGTH_DOWNLOAD>() << std::endl;
    shr::CurlSlist cookie = easyHandle.GetInfo<CURLINFO::CURLINFO_COOKIELIST>();

    multiHandle.RemoveEasyHandle(easyHandle);
    std::fclose(pFile);
    std::fclose(pFileDebug);
}

END_SHARELIBTEST_NAMESPACE
