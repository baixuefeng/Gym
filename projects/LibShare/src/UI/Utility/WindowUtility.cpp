#include "targetver.h"
#include "UI/Utility/WindowUtility.h"
#include <cassert>
#include <algorithm>
#include <boost/dll.hpp>
#include <CommCtrl.h>
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
#include <ShellScalingAPI.h>
#endif

//下面是使用 common control 所必须的
#pragma comment(lib, "Comctl32.lib")

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

SHARELIB_BEGIN_NAMESPACE

bool InitCmnCtrls()
{
	INITCOMMONCONTROLSEX commonCtrls = { sizeof(INITCOMMONCONTROLSEX) };
	commonCtrls.dwICC = ICC_WIN95_CLASSES
		| ICC_DATE_CLASSES
		| ICC_USEREX_CLASSES
		| ICC_COOL_CLASSES
		| ICC_INTERNET_CLASSES
		| ICC_PAGESCROLLER_CLASS
		| ICC_NATIVEFNTCTL_CLASS
		| ICC_STANDARD_CLASSES
		| ICC_LINK_CLASS;
	if (!::InitCommonControlsEx(&commonCtrls))
	{
		return false;
	}
	return true;
}

bool IsDialogBox(HWND hWnd)
{
    //下面是从atl中提取的源代码
    assert(::IsWindow(hWnd));
    TCHAR szBuf[8]; // "#32770" + NUL character
    if (GetClassName(hWnd, szBuf, sizeof(szBuf) / sizeof(szBuf[0])) == 0)
        return false;
    return lstrcmp(szBuf, _T("#32770")) == 0;
}

namespace win_dpi
{
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
    /** 使用boost::dll::import的时候，不要用函数指针类型，要用函数类型，因为从shared_library中get到的是对象的引用。
    */
    using TSetDpiFunc = HRESULT(__stdcall)(PROCESS_DPI_AWARENESS value);
    using TGetDpiFunc = HRESULT(__stdcall)(HANDLE hprocess, PROCESS_DPI_AWARENESS *value);
#endif

    bool SetDpiAwareness(DpiAwarenessType type)
    {
        if (type == DpiAwarenessType::ERROR_VALUE)
        {
            return false;
        }
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
        try
        {
            auto libFunc = boost::dll::import<TSetDpiFunc>(L"Shcore.dll", "SetProcessDpiAwareness", boost::dll::load_mode::search_system_folders);
            return SUCCEEDED(libFunc(static_cast<PROCESS_DPI_AWARENESS>(static_cast<int>(type))));
        }
        catch (...)
        {
        }
#endif
        return false;
    }

    DpiAwarenessType GetDpiAwareness()
    {
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
        try
        {
            auto libFunc = boost::dll::import<TGetDpiFunc>(L"Shcore.dll", "GetProcessDpiAwareness", boost::dll::load_mode::search_system_folders);
            PROCESS_DPI_AWARENESS dpi{};
            if (SUCCEEDED(libFunc(::GetCurrentProcess(), &dpi)))
            {
                return static_cast<DpiAwarenessType>(static_cast<int>(dpi));
            }
        }
        catch (...)
        {
        }
#endif
        return DpiAwarenessType::ERROR_VALUE;
    }

    uint32_t GetSystemDPIValue()
    {
        ATL::CRegKey reg;
        if (ERROR_SUCCESS == reg.Open(HKEY_CURRENT_USER, LR"(Control Panel\Desktop\WindowMetrics)"))
        {
            DWORD value = 0;
            if (ERROR_SUCCESS == reg.QueryDWORDValue(L"AppliedDPI", value))
            {
                return value;
            }
        }
        return 96;
    }
}
/////////////////////////////////////////////////////////////////////////////////////

BorderlessWindowHandler::BorderlessWindowHandler()
    : m_bEnableDragChangeSize(false)
{
    m_rcDragRegin = CRect( 10, 10, 10, 10);
}

void BorderlessWindowHandler::ShowWindowFullScreen()
{
    m_curMsgWindow.GetWindowRect(&m_rcRestoreFullScreen);
    CRect rcFullScreen = GetZoomedRect(m_curMsgWindow, true);
    m_curMsgWindow.SetWindowPos(HWND_TOPMOST, &rcFullScreen, 0);
}

bool BorderlessWindowHandler::IsFullScreen()
{
    CRect rcFullScreen = GetZoomedRect(m_curMsgWindow, true);
    CRect rcWindow;
    m_curMsgWindow.GetWindowRect(&rcWindow);
    return ((rcFullScreen == rcWindow) && 
        (m_curMsgWindow.GetWindowLongPtr(GWL_EXSTYLE) & WS_EX_TOPMOST));
}

void BorderlessWindowHandler::RestoreFromFullScreen()
{
    if (IsFullScreen() && !m_rcRestoreFullScreen.IsRectEmpty())
    {
        m_curMsgWindow.SetWindowPos(HWND_NOTOPMOST, &m_rcRestoreFullScreen, 0);
        m_rcRestoreFullScreen.SetRectEmpty();
    }
}

void BorderlessWindowHandler::SetEnableDragChangeSize(bool bEnable)
{
    m_bEnableDragChangeSize = bEnable;
}

void BorderlessWindowHandler::SetDragChangeSizeRegion(const CRect & rcRegion)
{
    m_rcDragRegin = rcRegion;
}

LRESULT BorderlessWindowHandler::OnBorderlessNcCalcSize(BOOL bCalcValidRects, LPARAM lParam)
{
    /* 无边框窗口的最大化功能:
    1.当窗口有WS_MAXIMIZEBOX风格时,最大化时会有额外的边框在屏幕工作区外边;
    2.当普通窗口时,最大化时需要修改窗口客户区的大小,不再保持整个窗口都是客户区,否则客户区超出屏幕,
      绘制内容也超出.这一点通过WM_NCCALCSIZE来实现.
    3.层窗口时,绘制(UpdateLayeredWindow)之后,窗口的大小会被修改成绘制时指定的大小,而且没有任何消息.
      如果保持窗口大小等于客户区大小,则窗口大小超出屏幕工作区,怎么绘制都不正确; 如果像普通窗口那样最大化
      不再保持全是客户区,那么如果按窗口区域绘制,因为窗口区域超出屏幕,导致绘制绘不出来;如果按客户区大小来绘制,
      绘制过后窗口大小被隐式缩小,客户区也会同步缩小,多次绘制后窗口越来越小;这里主要的差异在于:窗口大小与
      客户区大小不一致,普通窗口绘制之后窗口大小不会被隐式改变,而层窗口会.
    4.层窗口解决办法:最大化时,保持窗口大小与客户区大小一致,但既不使用窗口大小也不使用客户区大小,使用计算出
      的屏幕工作区大小.体现在代码中有3处:(1)OnSize中对size修正,调整控件响应区域,
      (2)绘制前画布大小进行修正;(3)UpdateLayeredWindow时,绘制区域修正.
    */
    if (bCalcValidRects)
    {
        if (m_curMsgWindow.IsZoomed() && 
            !(m_curMsgWindow.GetWindowLongPtr(GWL_STYLE) & WS_CHILD) && 
            !(m_curMsgWindow.GetWindowLongPtr(GWL_EXSTYLE) & WS_EX_LAYERED))
        {
            //客户区大小修正
            ((LPNCCALCSIZE_PARAMS)lParam)->rgrc[0] = GetZoomedRect(m_curMsgWindow);
        }
    }
    //全部都为客户区
    return 0;
}

BOOL BorderlessWindowHandler::OnBorderlessNcActivate(BOOL /*bActive*/)
{
    //避免画出标题栏
    return TRUE;
}

BOOL BorderlessWindowHandler::OnBorderlessEraseBkgnd(WTL::CDCHandle /*dc*/)
{
    return TRUE;
}

void BorderlessWindowHandler::OnBorderlessNcPaint(WTL::CRgnHandle /*rgn*/)
{
    //非客户区不绘制, 避免画出边框
}

void BorderlessWindowHandler::OnBorderlessSize(UINT nType, CSize /*size*/)
{
    if (nType != SIZE_MINIMIZED)
    {
        CRect rcRgn;
        m_curMsgWindow.GetWindowRect(&rcRgn);
        if (nType == SIZE_MAXIMIZED)
        {
            //最大化时可能会有边框在窗口客户区外面, 因此计算屏幕工作区大小, 把边框部分排除在外
            CRect rcMaximized = GetZoomedRect(m_curMsgWindow, false);
            rcMaximized.MoveToXY(0, 0);
            if (rcRgn.left < 0)
            {
                rcMaximized.MoveToX(-rcRgn.left);
            }
            if (rcRgn.top < 0)
            {
                rcMaximized.MoveToY(-rcRgn.top);
            }
            rcRgn = rcMaximized;
        }
        else
        {
            rcRgn.MoveToXY(0, 0);
        }

        //替换默认的区域,避免绘制出原始的圆角标题栏
        WTL::CRgnHandle rgn;
        rgn.CreateRectRgnIndirect(&rcRgn);
        m_curMsgWindow.SetWindowRgn(rgn);
    }

    SetMsgHandled(FALSE);
}

void BorderlessWindowHandler::OnBorderlessGetMinMaxInfo(LPMINMAXINFO lpMMI)
{
    CRect rcZoomed = GetZoomedRect(m_curMsgWindow);
    lpMMI->ptMaxPosition.x = 0;
    lpMMI->ptMaxPosition.y = 0;
    lpMMI->ptMaxSize.x = rcZoomed.Width();
    lpMMI->ptMaxSize.y = rcZoomed.Height();
    SetMsgHandled(FALSE);
}

LRESULT BorderlessWindowHandler::OnBorderlessNcHitTest(CPoint point)
{
    //该消息用来实现"拖拉改变窗口大小"的功能
    if (!m_bEnableDragChangeSize ||
        m_rcDragRegin.IsRectNull() ||
        m_curMsgWindow.IsZoomed() ||
        IsFullScreen())
    {
        SetMsgHandled(FALSE);
        return 0;
    }

    CRect rct;
    m_curMsgWindow.GetWindowRect(&rct);
    point.Offset(-rct.left, -rct.top);
    if (point.x >= 0 && point.x < m_rcDragRegin.left)
    {
        if (point.y >= 0 && point.y < m_rcDragRegin.top)
        {
            //左上
            return HTTOPLEFT;
        }
        else if (point.y >= m_rcDragRegin.top && point.y < (rct.Height() - m_rcDragRegin.bottom))
        {
            //左
            return HTLEFT;
        }
        else if ((point.y >= (rct.Height() - m_rcDragRegin.bottom)) && point.y < rct.Height())
        {
            //左下
            return HTBOTTOMLEFT;
        }
    }
    else if (point.x >= (rct.Width() - m_rcDragRegin.right) && point.x < rct.Width())
    {
        if (point.y >= 0 && point.y < m_rcDragRegin.top)
        {
            //右上
            return HTTOPRIGHT;
        }
        else if (point.y >= m_rcDragRegin.top && point.y < rct.Height() - m_rcDragRegin.bottom)
        {
            //右
            return HTRIGHT;
        }
        else if ((point.y >= (rct.Height() - m_rcDragRegin.bottom)) && point.y < rct.Height())
        {
            //右下
            return HTBOTTOMRIGHT;
        }
    }
    else
    {
        if (point.y >= 0 && point.y < m_rcDragRegin.top)
        {
            //上
            return HTTOP;
        }
        else if (point.y >= (rct.Height() - m_rcDragRegin.bottom) && point.y < rct.Height())
        {
            //下
            return HTBOTTOM;
        }
    }
    SetMsgHandled(FALSE);
    return 0;
}

CRect BorderlessWindowHandler::GetBorderlessDrawRect(HWND hWnd)
{
    if (!::IsWindow(hWnd))
    {
        return CRect();
    }
    if (::IsZoomed(hWnd))
    {
        return GetZoomedRect(hWnd, false);
    }
    else
    {
        CRect rcWindow;
        ::GetWindowRect(hWnd, &rcWindow);
        return rcWindow;
    }
}

CRect BorderlessWindowHandler::GetZoomedRect(HWND hWnd, bool bFullScreen /*= false*/)
{
    assert(::IsWindow(hWnd));
    HMONITOR hMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    assert(hMonitor);
    MONITORINFO mInfo{ sizeof(MONITORINFO) };
    BOOL isOk = ::GetMonitorInfo(hMonitor, &mInfo);
    assert(isOk);
    UNREFERENCED_PARAMETER(isOk);
    if (bFullScreen)
    {
        return mInfo.rcMonitor;
    }
    else
    {
        return mInfo.rcWork;
    }
}

//------------------------------------------------------------------------------------

MinRestoreHandler::MinRestoreHandler()
{
}

void MinRestoreHandler::OnMinRestoreWindowPosChanged(LPWINDOWPOS lpWndPos)
{
    if (m_curMsgWindow.IsWindow() && (lpWndPos->flags & SWP_NOCOPYBITS))
    {
        if (m_curMsgWindow.IsIconic())
        {
            OnMinimized(lpWndPos);
        }
        else
        {
            OnRestore(lpWndPos);
        }
    }
    SetMsgHandled(FALSE);
}

//------------------------------------------------------------------------------------

DragFrameHandler::DragFrameHandler()
{
}

LRESULT DragFrameHandler::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    assert(m_curMsgWindow.IsWindow());
    if ((wParam == (SC_SIZE | WMSZ_TOP)) ||
        (wParam == (SC_SIZE | WMSZ_LEFT)) ||
        (wParam == (SC_SIZE | WMSZ_RIGHT)) ||
        (wParam == (SC_SIZE | WMSZ_BOTTOM)) ||
        (wParam == (SC_SIZE | WMSZ_TOPLEFT)) ||
        (wParam == (SC_SIZE | WMSZ_TOPRIGHT)) ||
        (wParam == (SC_SIZE | WMSZ_BOTTOMLEFT)) ||
        (wParam == (SC_SIZE | WMSZ_BOTTOMRIGHT)) ||
        (wParam == (SC_MOVE | HTCAPTION)))
    {
        BOOL bDragFullWindow = FALSE;
        ::SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &bDragFullWindow, 0);
        if (bDragFullWindow)
        {
            ::SystemParametersInfo(SPI_SETDRAGFULLWINDOWS, FALSE, NULL, 0);
            auto res = ::DefWindowProc(m_curMsgWindow, uMsg, wParam, lParam);
            ::SystemParametersInfo(SPI_SETDRAGFULLWINDOWS, TRUE, NULL, 0);
            return res;
        }
    }
    SetMsgHandled(FALSE);
    return 0;
}

SHARELIB_END_NAMESPACE
