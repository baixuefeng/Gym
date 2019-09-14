#include "targetver.h"
#include "UI/Utility/NotifyIconMgr.h"
#include <cstring>
#include <cassert>
#include <mutex>
#include <utility>
#include <strsafe.h>
#include <Shlwapi.h>

//确保所有成员可用
#pragma warning(push)
#pragma warning(disable: 4005)

#if (NTDDI_VERSION < NTDDI_VISTA)
#define NTDDI_VERSION NTDDI_VISTA
#endif

#include <shellapi.h>
#pragma warning(pop)
#pragma comment(lib, "Shell32.lib")

SHARELIB_BEGIN_NAMESPACE

struct NotifyIconMgr::IconData: public NOTIFYICONDATA
{
    IconData()
    {
        std::memset(this, 0, sizeof(*this));
    }
};

NotifyIconMgr::NotifyIconMgr()
{
    m_pImpl = new IconData;
}

NotifyIconMgr::~NotifyIconMgr()
{
    if (m_pImpl)
    {
        delete m_pImpl;
        m_pImpl = nullptr;
    }
}

bool NotifyIconMgr::Initialize()
{
    if (m_pImpl->cbSize != 0)
    {
        return true;
    }
    if (NotifyIconMgr::IsDllVersionVistaOrLater())
    {
        m_pImpl->cbSize = sizeof(NOTIFYICONDATA);
    }
    else
    {
        m_pImpl->cbSize = NOTIFYICONDATA_V3_SIZE;
    }
    m_pImpl->uVersion = NOTIFYICON_VERSION_4;
    return true;
}

NotifyIconMgr::NotifyIconMgr(const NotifyIconMgr& other)
{
    m_pImpl = new IconData;
    *m_pImpl = *other.m_pImpl;
}

NotifyIconMgr::NotifyIconMgr(NotifyIconMgr && other)
{
    assert(!m_pImpl);
    m_pImpl = other.m_pImpl;
    other.m_pImpl = nullptr;
}

NotifyIconMgr& NotifyIconMgr::operator =(const NotifyIconMgr& other)
{
    if (this != &other)
    {
        *m_pImpl = *other.m_pImpl;
    }
    return *this;
}

NotifyIconMgr& NotifyIconMgr::operator=(NotifyIconMgr && other)
{
    if (this != &other)
    {
        std::swap(m_pImpl, other.m_pImpl);
    }
    return *this;
}

void NotifyIconMgr::ClearConfig()
{
    HWND hWnd = m_pImpl->hWnd;
    UINT uID = m_pImpl->uID;
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
    GUID iconGuid{ 0 };
    if (m_pImpl->uFlags & NIF_GUID)
    {
        iconGuid = m_pImpl->guidItem;
    }
#endif

    *m_pImpl = IconData();

    m_pImpl->hWnd = hWnd;
    m_pImpl->uID = uID;
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
    if (iconGuid != GUID{ 0 })
    {
        m_pImpl->uFlags |= NIF_GUID;
        m_pImpl->guidItem = iconGuid;
    }
#endif
}

void NotifyIconMgr::SetNotifyWndAndMsg(HWND hNotifyWnd, UINT uMsg)
{
    m_pImpl->hWnd = hNotifyWnd;
    if (uMsg != WM_NULL)
    {
        m_pImpl->uFlags |= NIF_MESSAGE;
        m_pImpl->uCallbackMessage = uMsg;
    }
}

void NotifyIconMgr::GetNotifyWndAndMsg(HWND & hNotifyWnd, UINT & uMsg)
{
    hNotifyWnd = m_pImpl->hWnd;
    uMsg = m_pImpl->uCallbackMessage;
}

void NotifyIconMgr::SetIcon(HICON icon, UINT nNotifyId)
{
    if (icon)
    {
        m_pImpl->uFlags |= NIF_ICON;
        m_pImpl->hIcon = icon;
    }
    m_pImpl->uID = nNotifyId;
}

void NotifyIconMgr::GetIcon(HICON & icon, UINT & nNotifyId)
{
    icon = m_pImpl->hIcon;
    nNotifyId = m_pImpl->uID;
}

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
void NotifyIconMgr::SetIcon(HICON icon, const GUID & notifyGuid)
{
    if (icon)
    {
        m_pImpl->uFlags |= NIF_ICON;
        m_pImpl->hIcon = icon;
    }
    m_pImpl->uFlags |= NIF_GUID;
    m_pImpl->guidItem = notifyGuid;
}

void NotifyIconMgr::GetIcon(HICON & icon, GUID & notifyGuid)
{
    icon = m_pImpl->hIcon;
    notifyGuid = m_pImpl->guidItem;
}

bool NotifyIconMgr::GetIconBoundingRect(RECT & rcBounds)
{
    NOTIFYICONIDENTIFIER iconId{ 0 };
    iconId.cbSize = sizeof(iconId);
    iconId.hWnd = m_pImpl->hWnd;
    iconId.uID = m_pImpl->uID;
    iconId.guidItem = m_pImpl->guidItem;
    return SUCCEEDED(::Shell_NotifyIconGetRect(&iconId, &rcBounds));
}
#endif

void NotifyIconMgr::SetTip(wchar_t* pTip)
{
    m_pImpl->uFlags |= NIF_TIP;
    if (IsDllVersionVistaOrLater())
    {
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
        m_pImpl->uFlags |= NIF_SHOWTIP;
#endif
    }
    ::StringCchCopy(m_pImpl->szTip, _countof(m_pImpl->szTip), pTip);
}

void NotifyIconMgr::SetInfoTitle(wchar_t * pInfoTitle)
{
    m_pImpl->uFlags |= NIF_INFO;
    ::StringCchCopy(m_pImpl->szInfoTitle, _countof(m_pImpl->szInfoTitle), pInfoTitle);
}

void NotifyIconMgr::SetInfo(wchar_t * pInfo)
{
    m_pImpl->uFlags |= NIF_INFO;
    ::StringCchCopy(m_pImpl->szInfo, _countof(m_pImpl->szInfo), pInfo);
}

void NotifyIconMgr::SetInfoIconType(DWORD iconType)
{
    m_pImpl->uFlags |= NIF_INFO;
    m_pImpl->dwInfoFlags = iconType;
}

DWORD NotifyIconMgr::GetInfoIconType()
{
    return m_pImpl->dwInfoFlags;
}

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
void NotifyIconMgr::SetBalloonIcon(HICON hBalloonIcon)
{
    m_pImpl->hBalloonIcon = hBalloonIcon;
}

HICON NotifyIconMgr::GetBalloonIcon()
{
    return m_pImpl->hBalloonIcon;
}

void NotifyIconMgr::SetRealTime(bool bRealTime)
{
    m_pImpl->uFlags |= NIF_INFO;
    if (bRealTime)
    {
        m_pImpl->uFlags |= NIF_REALTIME;
    }
    else
    {
        m_pImpl->uFlags &= ~NIF_REALTIME;
    }
}

bool NotifyIconMgr::GetRealTime()
{
    return !!(m_pImpl->uFlags & NIF_REALTIME);
}

void NotifyIconMgr::SetShowStandardTooltip(bool bStandard)
{
    if (bStandard)
    {
        m_pImpl->uFlags |= NIF_SHOWTIP;
    }
    else
    {
        m_pImpl->uFlags &= ~NIF_SHOWTIP;
    }
}

bool NotifyIconMgr::GetShowStandardTooltip()
{
    return !!(m_pImpl->uFlags & NIF_SHOWTIP);
}
#endif

void NotifyIconMgr::SetShow(bool bShow)
{
    m_pImpl->uFlags |= NIF_STATE;
    m_pImpl->dwStateMask |= NIS_HIDDEN;
    if (bShow)
    {
        m_pImpl->dwState &= ~NIS_HIDDEN;
    }
    else
    {
        m_pImpl->dwState |= NIS_HIDDEN;
    }
}

bool NotifyIconMgr::GetShow()
{
    return !(m_pImpl->dwState & NIS_HIDDEN);
}

bool NotifyIconMgr::Add()
{
    if (::Shell_NotifyIcon(NIM_ADD, m_pImpl))
    {
        return !!::Shell_NotifyIcon(NIM_SETVERSION, m_pImpl);
    }
    return false;
}

bool NotifyIconMgr::Modify()
{
    return !!::Shell_NotifyIcon(NIM_MODIFY, m_pImpl);
}

bool NotifyIconMgr::Delete()
{
    return !!::Shell_NotifyIcon(NIM_DELETE, m_pImpl);
}

bool NotifyIconMgr::SetFocus()
{
    return !!::Shell_NotifyIcon(NIM_SETFOCUS, m_pImpl);
}

bool NotifyIconMgr::IsDllVersionVistaOrLater()
{
    static bool s_isVistaOrLater = false;
    static std::once_flag s_dllVersionFlags;
    std::call_once(s_dllVersionFlags,
        []()
    {
        HMODULE hDll = ::LoadLibraryW(L"Shell32.dll");
        assert(hDll);
        if (hDll)
        {
            DLLGETVERSIONPROC pFn = (DLLGETVERSIONPROC)::GetProcAddress(hDll, "DllGetVersion");
            assert(pFn);
            DLLVERSIONINFO dllVer{ sizeof(dllVer) };
            if (pFn)
            {
                pFn(&dllVer);
            }
            ::FreeLibrary(hDll);
            if (dllVer.dwMajorVersion >= 6)
            {
                if ((dllVer.dwMinorVersion == 0)
                    && (dllVer.dwBuildNumber < 6))
                {
                    s_isVistaOrLater = false;
                }
                else
                {
                    s_isVistaOrLater = true;
                }
            }
        }
    });
    return s_isVistaOrLater;
}

SHARELIB_END_NAMESPACE
