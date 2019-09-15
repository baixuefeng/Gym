#pragma once
#include <atomic>
#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>
#include "MacroDefBase.h"
#include "curl/curl.h"

SHARELIB_BEGIN_NAMESPACE

//初始化全局的libcurl库
bool InitGlobalLibcurl();

//curl_slist辅助类
class CurlSlist
{
    SHARELIB_DISABLE_COPY_CLASS(CurlSlist);

public:
    CurlSlist();
    CurlSlist(curl_slist *pList);
    ~CurlSlist();
    CurlSlist(CurlSlist &&other);
    CurlSlist &operator=(CurlSlist &&other);
    operator curl_slist *() const;
    curl_slist *Data() const;
    void Attatch(curl_slist *pList);
    curl_slist *Detach();

    bool Append(const char *pData);
    void FreeAll();

private:
    curl_slist *m_pSlist;
};

//----------------------------------------------------------

//信息类型辅助类
template<CURLINFO infoIndex>
struct CurlInfoTypeHelper;

//----------------------------------------------------------

class CurlEasyHandle
{
    SHARELIB_DISABLE_COPY_CLASS(CurlEasyHandle);

public:
    CurlEasyHandle();
    ~CurlEasyHandle();
    CurlEasyHandle(CurlEasyHandle &&other);
    CurlEasyHandle &operator=(CurlEasyHandle &&other);
    operator CURL *() const;

    //重置所有设置到默认状态
    void ResetAllOption();

    //获取错误代码
    CURLcode GetLastErrCode();

    //设置是否有header或body, 默认bHeader = false, bBody = true
    bool SetHeaderBody(bool bHeader, bool bBody);

    //设置地址
    bool SetUrl(const char *pUrl);

    //设置是否允许3xx跳转，默认false
    bool SetFollowLocation(bool bEnable);

    /* 设置http头
    The headers must not be CRLF-terminated, because libcurl adds CRLF after each header item. 
     */
    bool AddHttpHeaders(std::initializer_list<const char *> iniList);
    bool AddHttpHeaders(const std::vector<std::string> &headers);

    /** 设置cookie
    @param[in] cookie
    */
    bool SetCookie(const char *pCookie);

    /* 设置http为POST方式, 并且设置Post字段, nSize为0表示pPostFields以\0结尾.
    格式类似于"name1=value1&name2=value2", 这种方式的POST不必设置读回调
     */
    bool SetPostFields(const char *pPostFields, size_t nSize = 0);

    /* 设置http为POST方式, post的内容用读回调传入, nPostSize用来指定上传的内容大小,
    如果为0表示使用 chunked transfer-encoding 方式, 并且会自动在http头中加入该字段
    这种方式的post必须设置读回调
     */
    bool SetPostWithCallback(size_t nPostSize = 0);

    /** SSL证书检查，只有当bVerifyPeer为true时，bVerifyHost为true才有效。
    When negotiating a TLS or SSL connection, the server sends a certificate indicating its identity. 
    Curl verifies whether the certificate is authentic, i.e. that you can trust that the server is who
    the certificate says it is. This trust is based on a chain of digital signatures, rooted in 
    certification authority (CA) certificates you supply. curl uses a default bundle of CA certificates
    (the path for that is determined at build time) and you can specify alternate certificates with the
    CURLOPT_CAINFO option or the CURLOPT_CAPATH option.

    When CURLOPT_SSL_VERIFYPEER is enabled, and the verification fails to prove that the certificate 
    is authentic, the connection fails. When the option is zero, the peer certificate verification
    succeeds regardless.

    Authenticating the certificate is not enough to be sure about the server. You typically also want 
    to ensure that the server is the server you mean to be talking to. Use CURLOPT_SSL_VERIFYHOST for 
    that. The check that the host name in the certificate is valid for the host name you're connecting 
    to is done independently of the CURLOPT_SSL_VERIFYPEER option.

    WARNING: disabling verification of the certificate allows bad guys to man-in-the-middle the 
    communication without you knowing it. Disabling verification makes the communication insecure. 
    Just having encryption on a transfer is not enough as you cannot be sure that you are communicating
    with the correct end-point.
    */
    bool SetSSLVerify(bool bVerifyPeer, bool bVerifyHost);

    /** 设置SSL证书，如果SetSSLVerify设置bVerifyPeer为false，则证书不必要提供
    */
    bool SetSSLCAPath(const char *pCAPath);

    /* 设置代理,比如"127.0.0.1:8888",这样Fiddler就可以截获libcurl收发的信息
    http://
    HTTP Proxy. Default when no scheme or proxy type is specified.
    https://
    HTTPS Proxy. (Added in 7.52.0 for OpenSSL, GnuTLS and NSS)
    socks4://
    SOCKS4 Proxy.
    socks4a://
    SOCKS4a Proxy. Proxy resolves URL hostname.
    socks5://
    SOCKS5 Proxy.
    socks5h://
    SOCKS5 Proxy. Proxy resolves URL hostname.
    */
    bool SetProxy(const char *pProxy);

    /** 设置HTTP头回调, 接收服务器响应
    @tparam[in] callableObj 调用原型:  Func(const char * pBuffer, size_t nBytes)
    */
    template<class Callable>
    bool SetHeaderCallback(Callable &&callableObj);

    /** 设置写回调, 接收服务器响应
    @tparam[in] callableObj 调用原型:  Func(const char * pBuffer, size_t nBytes)
    */
    template<class Callable>
    bool SetWriteCallback(Callable &&callableObj);

    /** 设置读回调, 上传或者POST时使用
    @tparam[in] callableObj 调用原型:  size_t Func(char * pOutBuffer, size_t nBufferSize);
        数据复制到pOutBurrfer中, pOutBuffer的大小为nBufferSize, 返回实际复制的数据大小
    */
    template<class Callable>
    bool SetReadCallback(Callable &&callableObj);

    /** 设置进度回调, 上传或者下载时使用
    @tparam[in] callableObj 调用原型: bool Func(int64_t nDownloadTotal, 
                                            int64_t nDownloadNow, 
                                            int64_t nUploadTotal, 
                                            int64_t nUploadNow);
        不使用的参数会传0, 比如只下载时, 上传参数nUploadTotal, nUploadNow 为0.
        返回true表示继续, 否则中断
    */
    template<class Callable>
    bool SetProgressCallback(Callable &&callableObj);

    /** 设置调试回调
    @tparam[in] callableObj 调用原型:  Func(curl_infotype type, const char * pBuffer, size_t nSize)
    */
    template<class Callable>
    bool SetDebugCallback(Callable &&callableObj);

    //获取信息
    template<CURLINFO infoIndex>
    auto GetInfo(typename CurlInfoTypeHelper<infoIndex>::type defaultValue =
                     std::decay_t<decltype(defaultValue)>{}) -> std::decay_t<decltype(defaultValue)>
    {
        //32位，当匹配double类型时，且该函数放到类外实现，就会崩溃；64位没问题，匹配其它类型没问题，或者原封不动放到类内也没问题。怪哉！
        if (m_pEasyHandle)
        {
            typename CurlInfoTypeHelper<infoIndex>::type info{};
            m_errCode = curl_easy_getinfo(m_pEasyHandle, infoIndex, &info);
            assert(m_errCode == CURLcode::CURLE_OK);
            if (m_errCode == CURLcode::CURLE_OK)
            {
                return info;
            }
        }
        return defaultValue;
    }

private:
    using header_callback = size_t (*)(char *pBuffer, size_t nSize, size_t nItems, void *pParam);

    //header_callback
    template<class CallableType>
    static size_t HeaderCallback(char *pBuffer, size_t nSize, size_t nItems, void *pParam);

    //curl_write_callback
    template<class CallableType>
    static size_t WriteCallback(char *pBuffer, size_t nSize, size_t nItems, void *pParam);

    //curl_read_callback
    template<class CallableType>
    static size_t ReadCallback(char *pBuffer, size_t nSize, size_t nItems, void *pParam);

    //curl_xferinfo_callback
    template<class CallableType>
    static int ProgressCallback(void *clientp,
                                curl_off_t dltotal,
                                curl_off_t dlnow,
                                curl_off_t ultotal,
                                curl_off_t ulnow);

    //curl_debug_callback
    template<class CallableType>
    static int DebugCallback(CURL *handle,       /* the handle/transfer this concerns */
                             curl_infotype type, /* what kind of data */
                             char *data,         /* points to the data */
                             size_t size,        /* size of the data pointed to */
                             void *pParam);      /* whatever the user please */

    CURL *m_pEasyHandle;
    CURLcode m_errCode;
    CurlSlist m_headerList;
    std::shared_ptr<void> m_spHeaderLambda;
    std::shared_ptr<void> m_spWriteLambda;
    std::shared_ptr<void> m_spReadLambda;
    std::shared_ptr<void> m_spProgressLambda;
    std::shared_ptr<void> m_spDebugLambda;
};

//----------------------------------------------------------

class CurlMultiHandle
{
    SHARELIB_DISABLE_COPY_CLASS(CurlMultiHandle);

public:
    CurlMultiHandle();
    ~CurlMultiHandle();
    operator CURLM *() const;
    CURLMcode GetLastErrCode();

    bool AddEasyHandle(const CurlEasyHandle &easyHandle);
    bool RemoveEasyHandle(const CurlEasyHandle &easyHandle);

    //执行结果
    enum TPerformResult
    {
        PERFORM_OK,       //成功
        PERFORM_FAILED,   //失败
        PERFORM_TIME_OUT, //超时
        PERFORM_TERMINATE //中止
    };

    /** 执行
    @param[in] nTimeOut 超时时间, 毫秒, 精度:100毫秒. 0表示无限
    */
    TPerformResult Perform(size_t nTimeOut = 0);

    //中止执行, 唯一的可以跨线程调用的接口
    void Terminate();

    std::vector<CURLMsg *> GetInfoRead();

private:
    CURLM *m_pMultiHandle;
    CURLMcode m_errCode;
    std::atomic<bool> m_bTerminate;
};

SHARELIB_END_NAMESPACE

#include "LibcurlWrapper.inl"
