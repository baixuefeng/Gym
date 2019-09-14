#pragma once
#include <cstdint>
#include <Windows.h>
#include <atlbase.h>
#include <atlwin.h>
#include <atltypes.h>
#include "WTL/atlapp.h"
#include "WTL/atlcrack.h"
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

// 初始化控件
bool InitCmnCtrls();

// 窗口是否为对话框
bool IsDialogBox(HWND hWnd);

namespace win_dpi
{
    enum DpiAwarenessType
    {
        ERROR_VALUE = -1,
        DPI_UNAWARE = 0,
        SYSTEM_DPI_AWARE = 1,
        PER_MONITOR_DPI_AWARE = 2
    };

    bool SetDpiAwareness(DpiAwarenessType type);

    DpiAwarenessType GetDpiAwareness();

    // 获取系统的DPI设置, 不受当前程序是否识别DPI的设置影响
    uint32_t GetSystemDPIValue();
}

// 把窗口改造为无边框窗口(全部都是客户区), 附带最大化、全屏、拖拉改变窗口大小的功能
class BorderlessWindowHandler
{
    SHARELIB_DISABLE_COPY_CLASS(BorderlessWindowHandler);

    BEGIN_MSG_MAP_EX(BorderlessWindowHandler)
        m_curMsgWindow = hWnd;
        MSG_WM_NCCALCSIZE(OnBorderlessNcCalcSize)//拦截此消息
        MSG_WM_NCACTIVATE(OnBorderlessNcActivate)//拦截此消息
        MSG_WM_ERASEBKGND(OnBorderlessEraseBkgnd)//拦截此消息
        MSG_WM_NCPAINT(OnBorderlessNcPaint)//拦截此消息
        MSG_WM_SIZE(OnBorderlessSize)//不拦截
        MSG_WM_GETMINMAXINFO(OnBorderlessGetMinMaxInfo)//不拦截
        MSG_WM_NCHITTEST(OnBorderlessNcHitTest)//只有当满足拖拉改变窗口大小的条件时才拦截
    END_MSG_MAP()

public:
    /** 构造函数
    */
    BorderlessWindowHandler();

//----全屏显示-----------------------

    /** 全屏显示
    */
    void ShowWindowFullScreen();

    /** 当前是否全屏
    */
    bool IsFullScreen();

    /** 从全屏恢复
    */
    void RestoreFromFullScreen();

//----拖拉改变窗口大小--------------------

    /** 设置是否允许拖拉改变窗口大小, 默认不允许
    @param[in] bEnable 是否允许
    */
    void SetEnableDragChangeSize(bool bEnable);

    /** 设置拖拉改变窗口大小的响应区域, 默认四边都为10
    @param[in] rcRegion 响应区域
    */
    void SetDragChangeSizeRegion(const CRect& rcRegion);

//----static------------------------------------------------

    /** 获取无边框窗口的绘制区域,屏幕坐标,兼容普通窗口、层窗口、最大化、全屏
    @param[in] hWnd 窗口句柄
    @return 窗口区域,屏幕坐标
    */
    static CRect GetBorderlessDrawRect(HWND hWnd);

    /** 获取最大化(全屏)时的区域, 屏幕坐标, 只适用于顶层窗口
    @param[in] hWnd 窗口句柄
    @param[in] bFullScreen true表示全屏,false表示最大化
    @return 窗口区域,屏幕坐标
    */
    static CRect GetZoomedRect(HWND hWnd, bool bFullScreen = false);

private:

    /** WM_NCCALCSIZE消息响应
    */
    LRESULT OnBorderlessNcCalcSize(BOOL /*bCalcValidRects*/, LPARAM /*lParam*/);

    /** WM_NCACTIVATE消息响应
    */
    BOOL OnBorderlessNcActivate(BOOL /*bActive*/);

    /** WM_ERASEBKGND消息响应
    */
    BOOL OnBorderlessEraseBkgnd(WTL::CDCHandle /*dc*/);

    /** WM_NCPAINT消息响应
    */
    void OnBorderlessNcPaint(WTL::CRgnHandle /*rgn*/);

    /** WM_SIZE消息响应
    */
    void OnBorderlessSize(UINT nType, CSize /*size*/);

    /** WM_GETMINMAXINFO消息响应
    */
    void OnBorderlessGetMinMaxInfo(LPMINMAXINFO lpMMI);

    /** WM_NCHITTEST消息响应
    */
    LRESULT OnBorderlessNcHitTest(CPoint point);

private:
    /** 当前消息窗口
    */
    ATL::CWindow m_curMsgWindow;

    /** 全屏恢复时的坐标
    */
    CRect m_rcRestoreFullScreen;

    /** 是否允许拖拉改变窗口大小
    */
    bool m_bEnableDragChangeSize;

    /** 拖拉改变窗口大小的区域
    */
    CRect m_rcDragRegin;
};

//---------------------------------------------------------------

// 最小化、还原的处理器，通过OnSize的nType判断是不准确的
class MinRestoreHandler
{
    SHARELIB_DISABLE_COPY_CLASS(MinRestoreHandler);

    BEGIN_MSG_MAP_EX(BorderlessWindowHandler)
        m_curMsgWindow = hWnd;
        MSG_WM_WINDOWPOSCHANGED(OnMinRestoreWindowPosChanged)//不拦截
    END_MSG_MAP()

public:
    MinRestoreHandler();

    virtual void OnMinimized(LPWINDOWPOS /*lpWndPos*/){};
    virtual void OnRestore(LPWINDOWPOS /*lpWndPos*/){};

private:
    void OnMinRestoreWindowPosChanged(LPWINDOWPOS lpWndPos);

private:
    //当前消息窗口
    ATL::CWindow m_curMsgWindow;
};

//----虚框拖拽处理器------------------------------------------------

class DragFrameHandler
{
    BEGIN_MSG_MAP_EX(DragFrameHandler)
        m_curMsgWindow = hWnd;
        MESSAGE_HANDLER_EX(WM_SYSCOMMAND, OnSysCommand)
    END_MSG_MAP()

public:
    /** 构造函数
    */
    DragFrameHandler();

    /** WM_SYSCOMMAND消息响应
    */
    LRESULT OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    /** 当前消息窗口
    */
    ATL::CWindow m_curMsgWindow;
};

SHARELIB_END_NAMESPACE
