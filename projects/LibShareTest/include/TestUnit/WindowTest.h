#pragma once
#include "UI/GraphicLayer/Layers/GraphicRootLayer.h"
#include "ui/Utility/AlphaMemDc.h"
#include "UI/Utility/NotifyIconMgr.h"

BEGIN_SHARELIBTEST_NAMESPACE

class TestWindow: 
    public ATL::CWindowImpl<TestWindow, ATL::CWindow, ATL::CWinTraits<WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_OVERLAPPEDWINDOW> >,
    public shr::MinRestoreHandler
{
    BEGIN_MSG_MAP_EX(TestWindow)
        CHAIN_MSG_MAP(shr::MinRestoreHandler)
        //MESSAGE_HANDLER(WM_PAINT, OnUserPaint)
        CHAIN_MSG_MAP_MEMBER(m_rootLayer)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_SIZE(OnSize)
        MSG_WM_KEYUP(OnKeyUp)
        MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
    END_MSG_MAP()

public:
    TestWindow();
    ~TestWindow();

protected:
    int OnCreate(LPCREATESTRUCT /*lpCreateStruct*/);
    void OnSize(UINT nType, CSize size);
    void OnPaint(CDCHandle dc);
    void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
    void OnLButtonDblClk(UINT nFlags, CPoint point);
    virtual void OnFinalMessage(_In_ HWND /*hWnd*/) override;
    LRESULT OnUserPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);

private:
    shr::GraphicRootLayer m_rootLayer;
    shr::AlphaMemDc m_memDc;
    shr::D2DRenderPack m_d2d;
    shr::DWriteUtility m_dwrite;

    shr::NotifyIconMgr m_notifyIcon;
};

END_SHARELIBTEST_NAMESPACE
