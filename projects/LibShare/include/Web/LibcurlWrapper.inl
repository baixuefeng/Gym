#include <cassert>
#include <type_traits>

SHARELIB_BEGIN_NAMESPACE

template<CURLINFO infoIndex>
struct CurlInfoTypeHelper
{
    static_assert(
        (infoIndex == CURLINFO_RESPONSE_CODE) || (infoIndex == CURLINFO_HTTP_CONNECTCODE) ||
            (infoIndex == CURLINFO_HTTP_VERSION) || (infoIndex == CURLINFO_FILETIME) ||
            (infoIndex == CURLINFO_REDIRECT_COUNT) || (infoIndex == CURLINFO_HEADER_SIZE) ||
            (infoIndex == CURLINFO_REQUEST_SIZE) || (infoIndex == CURLINFO_SSL_VERIFYRESULT) ||
            (infoIndex == CURLINFO_PROXY_SSL_VERIFYRESULT) ||
            (infoIndex == CURLINFO_HTTPAUTH_AVAIL) || (infoIndex == CURLINFO_PROXYAUTH_AVAIL) ||
            (infoIndex == CURLINFO_OS_ERRNO) || (infoIndex == CURLINFO_NUM_CONNECTS) ||
            (infoIndex == CURLINFO_PRIMARY_PORT) || (infoIndex == CURLINFO_LOCAL_PORT) ||
            //(infoIndex == CURLINFO_LASTSOCKET) || //不推荐
            (infoIndex == CURLINFO_CONDITION_UNMET) || (infoIndex == CURLINFO_RTSP_CLIENT_CSEQ) ||
            (infoIndex == CURLINFO_RTSP_SERVER_CSEQ) || (infoIndex == CURLINFO_RTSP_CSEQ_RECV) ||
            (infoIndex == CURLINFO_PROTOCOL),
        "CURLINFO type error!");
    using type = long;
};

template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_EFFECTIVE_URL>
{
    using type = const char *;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_REDIRECT_URL>
{
    using type = const char *;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_CONTENT_TYPE>
{
    using type = const char *;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_PRIVATE>
{
    using type = const char *;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_PRIMARY_IP>
{
    using type = const char *;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_LOCAL_IP>
{
    using type = const char *;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_FTP_ENTRY_PATH>
{
    using type = const char *;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_RTSP_SESSION_ID>
{
    using type = const char *;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_SCHEME>
{
    using type = const char *;
};

template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_NAMELOOKUP_TIME>
{
    using type = double;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_CONNECT_TIME>
{
    using type = double;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_APPCONNECT_TIME>
{
    using type = double;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_PRETRANSFER_TIME>
{
    using type = double;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_STARTTRANSFER_TIME>
{
    using type = double;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_TOTAL_TIME>
{
    using type = double;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_REDIRECT_TIME>
{
    using type = double;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_SIZE_UPLOAD>
{
    using type = double;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_SIZE_UPLOAD_T>
{
    using type = curl_off_t;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_SIZE_DOWNLOAD>
{
    using type = double;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_SIZE_DOWNLOAD_T>
{
    using type = curl_off_t;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_SPEED_DOWNLOAD>
{
    using type = double;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_SPEED_DOWNLOAD_T>
{
    using type = curl_off_t;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_SPEED_UPLOAD>
{
    using type = double;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_SPEED_UPLOAD_T>
{
    using type = curl_off_t;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_CONTENT_LENGTH_DOWNLOAD>
{
    using type = double;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_CONTENT_LENGTH_DOWNLOAD_T>
{
    using type = curl_off_t;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_CONTENT_LENGTH_UPLOAD>
{
    using type = double;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_CONTENT_LENGTH_UPLOAD_T>
{
    using type = curl_off_t;
};

template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_SSL_ENGINES>
{
    using type = curl_slist *;
};
template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_COOKIELIST>
{
    using type = curl_slist *;
};

template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_ACTIVESOCKET>
{
    using type = curl_socket_t;
};

template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_CERTINFO>
{
    using type = curl_certinfo *;
};

template<>
struct CurlInfoTypeHelper<CURLINFO::CURLINFO_TLS_SSL_PTR>
{
    using type = curl_tlssessioninfo *;
};
//template<>struct CurlInfoTypeHelper<CURLINFO::CURLINFO_TLS_SESSION>{ using type = curl_tlssessioninfo *; }; //不推荐

//--------------------------------------------------------------------------------------

template<class Callable>
bool CurlEasyHandle::SetHeaderCallback(Callable &&callableObj)
{
    if (m_pEasyHandle) {
        using CallableType = std::decay<Callable>::type;
        m_spHeaderLambda = std::make_shared<CallableType>(std::move(callableObj));
        if (curl_easy_setopt(m_pEasyHandle,
                             CURLOPT_HEADERFUNCTION,
                             (header_callback)(&HeaderCallback<CallableType>)) !=
            CURLcode::CURLE_OK) {
            return false;
        }
        return (curl_easy_setopt(m_pEasyHandle, CURLOPT_HEADERDATA, m_spHeaderLambda.get()) ==
                CURLcode::CURLE_OK);
    }
    return false;
}

template<class Callable>
bool CurlEasyHandle::SetWriteCallback(Callable &&callableObj)
{
    if (m_pEasyHandle) {
        using CallableType = std::decay<Callable>::type;
        m_spWriteLambda = std::make_shared<CallableType>(std::move(callableObj));
        if (curl_easy_setopt(m_pEasyHandle,
                             CURLOPT_WRITEFUNCTION,
                             (curl_write_callback)(&WriteCallback<CallableType>)) !=
            CURLcode::CURLE_OK) {
            return false;
        }
        return (curl_easy_setopt(m_pEasyHandle, CURLOPT_WRITEDATA, m_spWriteLambda.get()) ==
                CURLcode::CURLE_OK);
    }
    return false;
}

template<class Callable>
bool CurlEasyHandle::SetReadCallback(Callable &&callableObj)
{
    if (m_pEasyHandle) {
        using CallableType = std::decay<Callable>::type;
        m_spReadLambda = std::make_shared<CallableType>(std::move(callableObj));
        if (curl_easy_setopt(m_pEasyHandle,
                             CURLOPT_READFUNCTION,
                             (curl_read_callback)(&ReadCallback<CallableType>)) !=
            CURLcode::CURLE_OK) {
            return false;
        }
        return (curl_easy_setopt(m_pEasyHandle, CURLOPT_READDATA, m_spReadLambda.get()) ==
                CURLcode::CURLE_OK);
    }
    return false;
}

template<class Callable>
bool CurlEasyHandle::SetProgressCallback(Callable &&callableObj)
{
    if (m_pEasyHandle) {
        using CallableType = std::decay<Callable>::type;
        m_spProgressLambda = std::make_shared<CallableType>(std::move(callableObj));
        if (curl_easy_setopt(m_pEasyHandle,
                             CURLOPT_XFERINFOFUNCTION,
                             (curl_xferinfo_callback)(&ProgressCallback<CallableType>)) !=
            CURLcode::CURLE_OK) {
            return false;
        }
        if (curl_easy_setopt(m_pEasyHandle, CURLOPT_XFERINFODATA, m_spProgressLambda.get()) !=
            CURLcode::CURLE_OK) {
            return false;
        }
        m_errCode = curl_easy_setopt(m_pEasyHandle, CURLOPT_NOPROGRESS, 0);
        assert(m_errCode == CURLcode::CURLE_OK);
        if (m_errCode != CURLcode::CURLE_OK) {
            return false;
        }
        return true;
    }
    return false;
}

template<class Callable>
bool CurlEasyHandle::SetDebugCallback(Callable &&callableObj)
{
    if (m_pEasyHandle) {
        using CallableType = std::decay<Callable>::type;
        m_spDebugLambda = std::make_shared<CallableType>(std::move(callableObj));
        if (curl_easy_setopt(m_pEasyHandle,
                             CURLOPT_DEBUGFUNCTION,
                             (curl_debug_callback)(&DebugCallback<CallableType>)) !=
            CURLcode::CURLE_OK) {
            return false;
        }
        if (curl_easy_setopt(m_pEasyHandle, CURLOPT_DEBUGDATA, m_spDebugLambda.get()) !=
            CURLcode::CURLE_OK) {
            return false;
        }
        return (curl_easy_setopt(m_pEasyHandle, CURLOPT_VERBOSE, 1) == CURLcode::CURLE_OK);
    }
    return false;
}

template<class CallableType>
size_t CurlEasyHandle::HeaderCallback(char *pBuffer, size_t nSize, size_t nItems, void *pParam)
{
    CallableType *pFunc = (CallableType *)pParam;
    (*pFunc)((const char *)pBuffer, nItems * nSize);
    return nItems * nSize;
}

template<class CallableType>
size_t CurlEasyHandle::WriteCallback(char *pBuffer, size_t nSize, size_t nItems, void *pParam)
{
    CallableType *pFunc = (CallableType *)pParam;
    (*pFunc)((const char *)pBuffer, nItems * nSize);
    return nItems * nSize;
}

template<class CallableType>
size_t CurlEasyHandle::ReadCallback(char *pBuffer, size_t nSize, size_t nItems, void *pParam)
{
    CallableType *pFunc = (CallableType *)pParam;
    return (*pFunc)(pBuffer, nItems * nSize);
}

template<class CallableType>
int CurlEasyHandle::ProgressCallback(void *clientp,
                                     curl_off_t dltotal,
                                     curl_off_t dlnow,
                                     curl_off_t ultotal,
                                     curl_off_t ulnow)
{
    CallableType *pFunc = (CallableType *)clientp;
    /* 
    Returning a non-zero value from this callback will cause 
    libcurl to abort the transfer and return CURLE_ABORTED_BY_CALLBACK.
     */
    if (dltotal > 0 || ultotal > 0) {
        return !(*pFunc)(dltotal, dlnow, ultotal, ulnow);
    } else {
        return 0;
    }
}

template<class CallableType>
int CurlEasyHandle::DebugCallback(CURL *handle,       /* the handle/transfer this concerns */
                                  curl_infotype type, /* what kind of data */
                                  char *data,         /* points to the data */
                                  size_t size,        /* size of the data pointed to */
                                  void *pParam)       /* whatever the user please */
{
    assert(handle);
    (void)handle;
    CallableType *pFunc = (CallableType *)pParam;
    (*pFunc)(type, (const char *)data, size);
    return 0;
}

SHARELIB_END_NAMESPACE
