#include "stdafx.h"
#include "TestUnit/WindowTest.h"
#include "Resource.h"
#include "ui/Utility/Matrix2D.h"
#include "UI/GraphicLayer/ConfigEngine/XmlResourceMgr.h"

BEGIN_SHARELIBTEST_NAMESPACE

TestWindow::TestWindow():
m_rootLayer(this)
{
    m_rootLayer.SetD2DRenderType(shr::D2DRenderPack::Auto);

    m_notifyIcon.Initialize();
    m_notifyIcon.SetIcon(::LoadIcon(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SHARELIBTEST)), 1);
    m_notifyIcon.SetTip(L"icon tip");
    m_notifyIcon.SetInfoIconType(shr::NotifyIconMgr::InfoIconType::WarningIcon);
    m_notifyIcon.SetInfoTitle(L"info title");
    m_notifyIcon.SetInfo(L"info text");
    m_notifyIcon.SetShow(true);
}

TestWindow::~TestWindow()
{
    m_rootLayer.SaveToXml(L"layers_save.xml");
}

int TestWindow::OnCreate(LPCREATESTRUCT /*lpCreateStruct*/)
{
    HICON hIcon = ::LoadIcon(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SHARELIBTEST));
    HICON hIconSmall = ::LoadIcon(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SMALL));
    SetIcon(hIcon, TRUE);
    SetIcon(hIconSmall, FALSE);

    m_rootLayer.SetEnableDragChangeSize(true);
    auto spSkins = shr::LoadSkinFromXml(L"skin.xml");
    m_rootLayer.SetSkinMap(spSkins);
    m_rootLayer.LoadFromXml(L"layers.xml");
    m_rootLayer.LoadLuaFromXml(L"layout.xml");

    CRect rcWindow = shr::BorderlessWindowHandler::GetBorderlessDrawRect(*this);
    m_memDc.Create(WTL::CClientDC(*this), rcWindow.Size());
    m_d2d.CreateD2DFactory();
    m_dwrite.CreateDWriteFactory();

    m_notifyIcon.SetNotifyWndAndMsg(*this, WM_USER + 1);
    m_notifyIcon.SetShow(true);
    m_notifyIcon.Add();
    return 0;
}

void TestWindow::OnSize(UINT nType, CSize /*size*/)
{
    if (nType != SIZE_MINIMIZED)
    {
    }
}

void TestWindow::OnKeyUp(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
    if (nChar == VK_ESCAPE)
    {
        PostMessage(WM_CLOSE);
        return;
    }
    else if (nChar == VK_RETURN)
    {
        if (IsZoomed())
        {
            ShowWindowAsync(SW_RESTORE);
        }
        else
        {
            ShowWindowAsync(SW_MAXIMIZE);
        }
        return;
    }
    SetMsgHandled(FALSE);
}

void TestWindow::OnLButtonDblClk(UINT /*nFlags*/, CPoint /*point*/)
{
    if (IsZoomed())
    {
        ShowWindow(SW_RESTORE);
    }
    else
    {
        ShowWindow(SW_MAXIMIZE);
    }
}

void TestWindow::OnFinalMessage(_In_ HWND /*hWnd*/)
{
    m_notifyIcon.Delete();
    PostQuitMessage(0);
}

LRESULT TestWindow::OnUserPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
    CPaintDC dc(*this);
    CRect rcWnd = shr::BorderlessWindowHandler::GetBorderlessDrawRect(*this);
    m_memDc.Resize(rcWnd.Size());
    auto nState = m_memDc.SaveDC();
    m_memDc.IntersectClipRect(&dc.m_ps.rcPaint);
    m_memDc.ClearZero();

    shr::TDWriteTextAttributes textAttr;
    textAttr.m_pFontName = L"微软雅黑";
    textAttr.m_fontSize = 40.f;
    auto spTextFormat = m_dwrite.CreateTextFormat(textAttr);
    ATL::CComPtr<IDWriteTextLayout> spTextLayout;

    const std::wstring str = L"abcd晃炒更之";
    m_dwrite.m_spDWriteFactory->CreateTextLayout(str.c_str(), (UINT32)str.size(), spTextFormat, FLT_MAX, FLT_MAX, &spTextLayout);


    m_memDc.BitBltDraw(dc);
    m_memDc.RestoreDC(nState);

    return 0;
}

END_SHARELIBTEST_NAMESPACE
