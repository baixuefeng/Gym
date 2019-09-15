#include <mutex>
#include "Web/LibcurlWrapper.h"
#include <chrono>
#include <thread>
#include <utility>
#include "curl/curl.h"

SHARELIB_BEGIN_NAMESPACE

#ifndef CHECK_EASY_ERRCODE
#    define CHECK_EASY_ERRCODE(err)                                                                \
        assert(err == CURLcode::CURLE_OK);                                                         \
        if (err != CURLcode::CURLE_OK)                                                             \
        {                                                                                          \
            return false;                                                                          \
        }
#endif

#ifndef CHECK_MULTI_ERRCODE
#    define CHECK_MULTI_ERRCODE(err)                                                               \
        assert(err == CURLMcode::CURLM_OK);                                                        \
        if (err != CURLMcode::CURLM_OK)                                                            \
        {                                                                                          \
            return false;                                                                          \
        }
#endif

struct AutoLibcurl
{
    AutoLibcurl() { m_initOk = (curl_global_init(CURL_GLOBAL_ALL) == CURLcode::CURLE_OK); }
    ~AutoLibcurl() { curl_global_cleanup(); }
    bool m_initOk;
};

bool InitGlobalLibcurl()
{
    static std::once_flag s_initFlag;
    static bool s_initOk;
    std::call_once(s_initFlag, []() {
        static AutoLibcurl s_initLibcurl;
        s_initOk = s_initLibcurl.m_initOk;
    });
    return s_initOk;
}

//----------------------------------------------------------

CurlSlist::CurlSlist()
    : m_pSlist(nullptr)
{}

CurlSlist::CurlSlist(curl_slist *pList)
{
    Attatch(pList);
}

CurlSlist::~CurlSlist()
{
    FreeAll();
}

void CurlSlist::Attatch(curl_slist *pList)
{
    assert(!m_pSlist);
    m_pSlist = pList;
}

curl_slist *CurlSlist::Detach()
{
    auto p = m_pSlist;
    m_pSlist = nullptr;
    return p;
}

CurlSlist::CurlSlist(CurlSlist &&other)
{
    m_pSlist = other.m_pSlist;
    other.m_pSlist = nullptr;
}

CurlSlist &CurlSlist::operator=(CurlSlist &&other)
{
    if (this != &other)
    {
        std::swap(m_pSlist, other.m_pSlist);
    }
    return *this;
}

CurlSlist::operator curl_slist *() const
{
    return m_pSlist;
}

curl_slist *CurlSlist::Data() const
{
    return m_pSlist;
}

bool CurlSlist::Append(const char *pData)
{
    m_pSlist = curl_slist_append(m_pSlist, pData);
    return (m_pSlist != nullptr);
}

void CurlSlist::FreeAll()
{
    if (m_pSlist)
    {
        curl_slist_free_all(m_pSlist);
    }
}

//----------------------------------------------------------

CurlEasyHandle::CurlEasyHandle()
    : m_pEasyHandle(curl_easy_init())
    , m_errCode(CURLcode::CURLE_OK)
{
    assert(m_pEasyHandle);
    curl_easy_setopt(m_pEasyHandle, CURLOPT_NOSIGNAL, 1);
}

CurlEasyHandle::CurlEasyHandle(CurlEasyHandle &&other)
{
    m_pEasyHandle = other.m_pEasyHandle;
    other.m_pEasyHandle = nullptr;

    m_errCode = other.m_errCode;

    m_headerList = std::move(other.m_headerList);
    m_spWriteLambda = std::move(other.m_spWriteLambda);
    m_spReadLambda = std::move(other.m_spReadLambda);
    m_spProgressLambda = std::move(other.m_spProgressLambda);
    m_spDebugLambda = std::move(other.m_spDebugLambda);
}

CurlEasyHandle &CurlEasyHandle::operator=(CurlEasyHandle &&other)
{
    if (&other != this)
    {
        std::swap(m_pEasyHandle, other.m_pEasyHandle);
        std::swap(m_errCode, other.m_errCode);
        std::swap(m_headerList, other.m_headerList);
        std::swap(m_spWriteLambda, other.m_spWriteLambda);
        std::swap(m_spReadLambda, other.m_spReadLambda);
        std::swap(m_spProgressLambda, other.m_spProgressLambda);
        std::swap(m_spDebugLambda, other.m_spDebugLambda);
    }
    return *this;
}

CurlEasyHandle::~CurlEasyHandle()
{
    if (m_pEasyHandle)
    {
        //结束调试回调
        curl_easy_setopt(m_pEasyHandle, CURLOPT_VERBOSE, 0);
        //结束进度回调
        curl_easy_setopt(m_pEasyHandle, CURLOPT_NOPROGRESS, 1);
        curl_easy_cleanup(m_pEasyHandle);
    }
}

CurlEasyHandle::operator CURL *() const
{
    return m_pEasyHandle;
}

void CurlEasyHandle::ResetAllOption()
{
    if (m_pEasyHandle)
    {
        curl_easy_reset(m_pEasyHandle);
    }

    m_errCode = CURLcode::CURLE_OK;

    m_headerList.FreeAll();
    m_spHeaderLambda.reset();
    m_spWriteLambda.reset();
    m_spReadLambda.reset();
    m_spProgressLambda.reset();
    m_spDebugLambda.reset();
}

CURLcode CurlEasyHandle::GetLastErrCode()
{
    return m_errCode;
}

bool CurlEasyHandle::SetHeaderBody(bool bHeader, bool bBody)
{
    if (m_pEasyHandle)
    {
        m_errCode = curl_easy_setopt(m_pEasyHandle, CURLOPT_HEADER, bHeader);
        CHECK_EASY_ERRCODE(m_errCode);
        m_errCode = curl_easy_setopt(m_pEasyHandle, CURLOPT_NOBODY, !bBody);
        CHECK_EASY_ERRCODE(m_errCode);
        return true;
    }
    return false;
}

bool CurlEasyHandle::SetUrl(const char *pUrl)
{
    if (m_pEasyHandle && pUrl)
    {
        m_errCode = curl_easy_setopt(m_pEasyHandle, CURLOPT_URL, pUrl);
        CHECK_EASY_ERRCODE(m_errCode);
        return true;
    }
    return false;
}

bool CurlEasyHandle::SetFollowLocation(bool bEnable)
{
    if (m_pEasyHandle)
    {
        m_errCode = curl_easy_setopt(m_pEasyHandle, CURLOPT_FOLLOWLOCATION, bEnable);
        CHECK_EASY_ERRCODE(m_errCode);
        return true;
    }
    return false;
}

bool CurlEasyHandle::AddHttpHeaders(std::initializer_list<const char *> iniList)
{
    if (!m_pEasyHandle)
    {
        return false;
    }
    for (auto &item : iniList)
    {
        m_headerList.Append(item);
    }
    m_errCode = curl_easy_setopt(m_pEasyHandle, CURLOPT_HTTPHEADER, m_headerList.Data());
    CHECK_EASY_ERRCODE(m_errCode);
    return true;
}

bool CurlEasyHandle::AddHttpHeaders(const std::vector<std::string> &headers)
{
    if (!m_pEasyHandle)
    {
        return false;
    }
    for (auto &item : headers)
    {
        m_headerList.Append(item.c_str());
    }
    m_errCode = curl_easy_setopt(m_pEasyHandle, CURLOPT_HTTPHEADER, m_headerList.Data());
    CHECK_EASY_ERRCODE(m_errCode);
    return true;
}

bool CurlEasyHandle::SetCookie(const char *pCookie)
{
    if (!m_pEasyHandle)
    {
        return false;
    }
    m_errCode = curl_easy_setopt(m_pEasyHandle, CURLOPT_COOKIE, pCookie);
    CHECK_EASY_ERRCODE(m_errCode);
    return true;
}

bool CurlEasyHandle::SetPostFields(const char *pPostFields, size_t nSize /*= 0*/)
{
    if (m_pEasyHandle)
    {
        m_errCode = curl_easy_setopt(m_pEasyHandle, CURLOPT_POSTFIELDS, pPostFields);
        CHECK_EASY_ERRCODE(m_errCode);
        if (nSize > 0)
        {
            m_errCode = curl_easy_setopt(m_pEasyHandle, CURLOPT_POSTFIELDSIZE_LARGE, nSize);
            CHECK_EASY_ERRCODE(m_errCode);
        }
        return true;
    }
    return false;
}

bool CurlEasyHandle::SetPostWithCallback(size_t nPostSize /*= 0*/)
{
    if (m_pEasyHandle)
    {
        m_errCode = curl_easy_setopt(m_pEasyHandle, CURLOPT_POST, 1);
        CHECK_EASY_ERRCODE(m_errCode);
        if (nPostSize > 0)
        {
            m_errCode = curl_easy_setopt(m_pEasyHandle, CURLOPT_POSTFIELDSIZE_LARGE, nPostSize);
            CHECK_EASY_ERRCODE(m_errCode);
            return true;
        }
        else
        {
            return AddHttpHeaders({"Transfer-Encoding: chunked"});
        }
    }
    return false;
}

bool CurlEasyHandle::SetSSLVerify(bool bVerifyPeer, bool bVerifyHost)
{
    if (m_pEasyHandle)
    {
        m_errCode = curl_easy_setopt(m_pEasyHandle, CURLOPT_SSL_VERIFYPEER, bVerifyPeer);
        CHECK_EASY_ERRCODE(m_errCode);
        bVerifyHost = (bVerifyPeer && bVerifyHost);
        m_errCode = curl_easy_setopt(m_pEasyHandle, CURLOPT_SSL_VERIFYHOST, bVerifyHost);
        CHECK_EASY_ERRCODE(m_errCode);
        return true;
    }
    return false;
}

bool CurlEasyHandle::SetSSLCAPath(const char *pCAPath)
{
    if (m_pEasyHandle)
    {
        m_errCode = curl_easy_setopt(m_pEasyHandle, CURLOPT_CAINFO, pCAPath);
        CHECK_EASY_ERRCODE(m_errCode);
        return true;
    }
    return false;
}

bool CurlEasyHandle::SetProxy(const char *pProxy)
{
    if (m_pEasyHandle)
    {
        m_errCode = curl_easy_setopt(m_pEasyHandle, CURLOPT_PROXY, pProxy);
        CHECK_EASY_ERRCODE(m_errCode);
        return true;
    }
    return false;
}

//----------------------------------------------------------

CurlMultiHandle::CurlMultiHandle()
    : m_pMultiHandle(curl_multi_init())
    , m_errCode(CURLMcode::CURLM_OK)
    , m_bTerminate(false)
{
    assert(m_pMultiHandle);
}

CurlMultiHandle::~CurlMultiHandle()
{
    if (m_pMultiHandle)
    {
        m_errCode = curl_multi_cleanup(m_pMultiHandle);
        assert(m_errCode == CURLMcode::CURLM_OK);
    }
}

CurlMultiHandle::operator CURLM *() const
{
    return m_pMultiHandle;
}

CURLMcode CurlMultiHandle::GetLastErrCode()
{
    return m_errCode;
}

bool CurlMultiHandle::AddEasyHandle(const CurlEasyHandle &easyHandle)
{
    if (m_pMultiHandle)
    {
        m_errCode = curl_multi_add_handle(m_pMultiHandle, easyHandle);
        CHECK_MULTI_ERRCODE(m_errCode);
        return true;
    }
    return false;
}

bool CurlMultiHandle::RemoveEasyHandle(const CurlEasyHandle &easyHandle)
{
    if (m_pMultiHandle)
    {
        m_errCode = curl_multi_remove_handle(m_pMultiHandle, easyHandle);
        CHECK_MULTI_ERRCODE(m_errCode);
        return true;
    }
    return false;
}

CurlMultiHandle::TPerformResult CurlMultiHandle::Perform(size_t nTimeOut /*= 0*/)
{
    if (!m_pMultiHandle)
    {
        return PERFORM_FAILED;
    }
    m_bTerminate = false;
    auto timePt = std::chrono::system_clock::now();
    for (int nStillRunning = INT_MAX, nRepeat = 0; (nStillRunning > 0) && !m_bTerminate;)
    {
        m_errCode = curl_multi_perform(m_pMultiHandle, &nStillRunning);
        if ((m_errCode != CURLMcode::CURLM_OK) || (nStillRunning == 0))
        {
            break;
        }
        int nCountOccurred = 0;
        m_errCode = curl_multi_wait(m_pMultiHandle, 0, 0, 50, &nCountOccurred);
        if (m_errCode != CURLMcode::CURLM_OK)
        {
            break;
        }

        if (nTimeOut > 0)
        {
            auto timeCur = std::chrono::system_clock::now();
            auto timeDuration =
                std::chrono::duration_cast<std::chrono::milliseconds>(timeCur - timePt).count();
            if (timeDuration > static_cast<decltype(timeDuration)>(nTimeOut))
            {
                return PERFORM_TIME_OUT;
            }
        }

        if (nCountOccurred > 0)
        {
            nRepeat = 0;
        }
        else
        {
            if (++nRepeat > 1)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
    }
    if (m_errCode != CURLMcode::CURLM_OK)
    {
        return PERFORM_FAILED;
    }
    else if (m_bTerminate)
    {
        return PERFORM_TERMINATE;
    }
    else
    {
        return PERFORM_OK;
    }
}

void CurlMultiHandle::Terminate()
{
    m_bTerminate = true;
}

std::vector<CURLMsg *> CurlMultiHandle::GetInfoRead()
{
    std::vector<CURLMsg *> msgs;
    if (m_pMultiHandle)
    {
        CURLMsg *pMsg = nullptr;
        do
        {
            int nMsgCount = 0;
            pMsg = curl_multi_info_read(m_pMultiHandle, &nMsgCount);
            if (pMsg)
            {
                msgs.push_back(pMsg);
            }
        } while (pMsg);
    }
    return std::move(msgs);
}

SHARELIB_END_NAMESPACE
