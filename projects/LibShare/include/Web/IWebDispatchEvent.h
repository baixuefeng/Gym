#pragma once
#include <atlbase.h>
#include <atlcom.h>
#include <exdispid.h>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

//假的控件id,这里并不使用,只不过ATL必须提供这个
#define SHARELIB_WEB_CONTROL_FAKEID 12345

class ATL_NO_VTABLE IWebDispatchEvent :
    public ATL::IDispEventImpl<SHARELIB_WEB_CONTROL_FAKEID, IWebDispatchEvent>
{
public:
    BEGIN_SINK_MAP(IWebDispatchEvent)
        SINK_ENTRY(SHARELIB_WEB_CONTROL_FAKEID, DISPID_BEFORENAVIGATE2, OnBeforeNavigate2)
        SINK_ENTRY(SHARELIB_WEB_CONTROL_FAKEID, DISPID_DOCUMENTCOMPLETE, OnDocumentComplete)
        SINK_ENTRY(SHARELIB_WEB_CONTROL_FAKEID, DISPID_NAVIGATECOMPLETE2, OnNavigateComplete2)
        SINK_ENTRY(SHARELIB_WEB_CONTROL_FAKEID, DISPID_NEWWINDOW3, OnNewWindow3)
        SINK_ENTRY(SHARELIB_WEB_CONTROL_FAKEID, DISPID_NAVIGATEERROR, OnNavigateError)
        SINK_ENTRY(SHARELIB_WEB_CONTROL_FAKEID, DISPID_WINDOWCLOSING, OnWindowClosing)
    END_SINK_MAP()

#undef SHARELIB_WEB_CONTROL_FAKEID

public:
    //标准IE事件回调接口
    //https://msdn.microsoft.com/en-us/library/aa768283(v=vs.85).aspx

    /** Fires before navigation occurs in the given object
    @param[in] pDisp interface for the WebBrowser object that represents the window or frame
    @param[in] pUrl VT_BSTR that contains the URL to navigate to
    @param[in] pFlags  VT_I4 that contains the following flag, or zero
    @param[in] pTargetFrameName VT_BSTR that contains the name of the frame in which to display the resource, or NULL
    @param[in] pPostData VT_BYREF|VT_VARIANT that contains the data to send to the server if the HTTP POST transaction is used
    @param[in] pHeaders VT_BSTR that contains additional HTTP headers to send to the server
    @param[out] pCancel VARIANT_BOOL that contains the cancel flag. An application can set this parameter to VARIANT_TRUE to cancel the navigation operation, or to VARIANT_FALSE to allow the navigation operation to proceed
    */
    virtual void WINAPI OnBeforeNavigate2(LPDISPATCH pDisp, ATL::CComVariant* pUrl, ATL::CComVariant* pFlags, ATL::CComVariant* pTargetFrameName, ATL::CComVariant* pPostData, ATL::CComVariant* pHeaders, ATL::CComVariant* pCancel)
    {
        (void)pDisp;
        (void)pUrl;
        (void)pFlags;
        (void)pTargetFrameName;
        (void)pPostData;
        (void)pHeaders;
        (void)pCancel;
    }

    /** Fires when a document is completely loaded and initialized.
    @param[in] pDisp interface for the WebBrowser object that represents the window or frame
    @param[in] pUrl Variant structure of type String that specifies the URL, UNC file name, or a PIDL of the loaded document
    */
    virtual void WINAPI OnDocumentComplete(LPDISPATCH pDisp, ATL::CComVariant* pUrl)
    {
        (void)pDisp;
        (void)pUrl;
    }

    /** Fires after a navigation to a link is completed on a window element or a frameSet element
    @param[in] pDisp interface for the WebBrowser object that represents the window or frame
    @param[in] pUrl Variant structure of type String that contains the URL, UNC file name, or PIDL that was navigated to
    */
    virtual void WINAPI OnNavigateComplete2(LPDISPATCH pDisp, ATL::CComVariant* pUrl)
    {
        (void)pDisp;
        (void)pUrl;
    }

    /** Raised when a new window is to be created
    @param[in,out] ppDisp An interface pointer that, optionally, receives the IDispatch interface pointer of a new WebBrowser object or an InternetExplorer object.
    @param[in,out] pCancel A Boolean value that determines whether the current navigation should be canceled, VARIANT_TRUE or VARIANT_FALSE.
    @param[in] dwFlags LONG,The flags from the NWMF enumeration that pertain to the new window.
    @param[in] pbstrUrlContext BSTR,The URL of the page that is opening the new window.
    @param[in] pbstrUrl BSTR,The URL that is opened in the new window.
    @return
    */
    virtual void WINAPI OnNewWindow3(LPDISPATCH* ppDisp, BOOLEAN* pCancel, LONG dwFlags, BSTR pbstrUrlContext, BSTR pbstrUrl)
    {
        (void)ppDisp;
        (void)pCancel;
        (void)dwFlags;
        (void)pbstrUrlContext;
        (void)pbstrUrl;
    }

    /** Fires when an error occurs during navigation
    @param[in] pDisp interface for the WebBrowser object that represents the window or frame
    @param[in] pURL VARIANT structure of type VT_BSTR that contains the URL for which navigation failed
    @param[in] pTargetFrameName  VT_BSTR that contains the name of the frame in which to display the resource, or NULL
    @param[in] pStatusCode VT_I4 containing an error status code
    @param[in] pCancel VARIANT structure of type VARIANT_BOOL that specifies whether to cancel the navigation to an error page or to any further autosearch.
    @return
    */
    virtual void WINAPI OnNavigateError(LPDISPATCH pDisp, ATL::CComVariant* pURL, ATL::CComVariant* pTargetFrameName, ATL::CComVariant* pStatusCode, ATL::CComVariant* pCancel)
    {
        (void)pDisp;
        (void)pURL;
        (void)pTargetFrameName;
        (void)pStatusCode;
        (void)pCancel;
    }

    /** Fires when the window of the object is about to be closed by script.
    @param[in] isChildWindow A Boolean that specifies whether the window was created from script.
    @param[in] pCancel A Boolean value that specifies whether the window is prevented from closing.
    */
    virtual void WINAPI OnWindowClosing(BOOLEAN isChildWindow, BOOLEAN* pCancel)
    {
        (void)isChildWindow;
        (void)pCancel;
    }
};

SHARELIB_END_NAMESPACE
