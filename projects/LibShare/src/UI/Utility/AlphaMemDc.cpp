#include "targetver.h"
#include "UI/Utility/AlphaMemDc.h"
#include <cassert>
#include <cmath>
#include <cstring>

//使用AlphaBlend相关的函数需要引入库
#pragma comment(lib, "Msimg32.lib")

SHARELIB_BEGIN_NAMESPACE

AlphaMemDc::AlphaMemDc()
{
    m_pBitmapData = nullptr;
    m_bitmapSize.cx = 0;
    m_bitmapSize.cy = 0;
    m_clearBrush = (HBRUSH)::GetStockObject(BLACK_BRUSH);
}

AlphaMemDc::~AlphaMemDc()
{
    Destroy();
}

bool AlphaMemDc::Create(HDC hDc, SIZE sz)
{
    if (IsNull())
    {
        if (CreateCompatibleDC(hDc))
        {
            //此处创建Bitmap必须用外部DC，因为刚创建的内存DC只有黑的一个点
            m_bitmap.Attach(CreateAlphaBitmap(hDc, sz.cx, sz.cy, (void **)&m_pBitmapData));
            if (m_bitmap)
            {
                m_bitmapSize = sz;
                m_oldBitmap.Attach(SelectBitmap(m_bitmap));
                ::SetGraphicsMode(m_hDC, GM_ADVANCED);
            }
            else
            {
                DeleteDC();
            }
        }
    }
    return !IsNull();
}

bool AlphaMemDc::Resize(SIZE sz)
{
    assert(!IsNull());
    if (IsNull())
    {
        return false;
    }
    else if ((m_bitmapSize.cx != sz.cx) || (m_bitmapSize.cy != sz.cy))
    {
        HBITMAP hBitmap = CreateAlphaBitmap(m_hDC, sz.cx, sz.cy, (void **)&m_pBitmapData);
        if (hBitmap)
        {
            SelectBitmap(hBitmap);
            m_bitmap.Attach(hBitmap);
            m_bitmapSize = sz;
            return true;
        }
        return false;
    }
    return true;
}

SIZE AlphaMemDc::GetDCSize()
{
    return m_bitmapSize;
}

uint8_t *AlphaMemDc::GetBitmapData()
{
    return (uint8_t *)m_pBitmapData;
}

void AlphaMemDc::ClearZero()
{
    RECT rc{0, 0, m_bitmapSize.cx, m_bitmapSize.cy};
    FillRect(&rc, m_clearBrush);
}

void AlphaMemDc::Destroy()
{
    if (!IsNull())
    {
        m_pBitmapData = nullptr;
        SelectBitmap(m_oldBitmap);
        DeleteDC();
        if (m_bitmap)
        {
            m_bitmap.DeleteObject();
        }
        m_bitmapSize.cx = 0;
        m_bitmapSize.cy = 0;
    }
}

bool AlphaMemDc::CopyBitmapTo(void *pDest, const RECT *pRect /*= nullptr*/)
{
    if (!pDest)
    {
        return false;
    }
    uint8_t *pBit = (uint8_t *)pDest;
    return TraversePixel(
        [&pBit](const POINT & /*pt*/, uint8_t &a, uint8_t &r, uint8_t &g, uint8_t &b) -> bool {
            *pBit++ = b;
            *pBit++ = g;
            *pBit++ = r;
            *pBit++ = a;
            return true;
        },
        pRect);
}

bool AlphaMemDc::AlphaDraw(HDC hDestDc,
                           const RECT *pDest /*= nullptr*/,
                           const RECT *pSrc /*= nullptr*/,
                           uint8_t alpha /*= 255*/,
                           bool bSrcAlpha /*= true*/)
{
    if (!IsNull() && hDestDc)
    {
        BLENDFUNCTION blend = {0};
        blend.BlendOp = AC_SRC_OVER;
        blend.BlendFlags = 0;
        blend.SourceConstantAlpha = alpha;
        blend.AlphaFormat = (bSrcAlpha ? AC_SRC_ALPHA : 0);
        return !!::AlphaBlend(hDestDc,
                              (pDest ? pDest->left : 0),
                              (pDest ? pDest->top : 0),
                              (pDest ? pDest->right - pDest->left : m_bitmapSize.cx),
                              (pDest ? pDest->bottom - pDest->top : m_bitmapSize.cy),
                              m_hDC,
                              (pSrc ? pSrc->left : 0),
                              (pSrc ? pSrc->top : 0),
                              (pSrc ? pSrc->right - pSrc->left : m_bitmapSize.cx),
                              (pSrc ? pSrc->bottom - pSrc->top : m_bitmapSize.cy),
                              blend);
    }
    else
    {
        return false;
    }
}

bool AlphaMemDc::BitBltDraw(HDC hDestDc,
                            const RECT *pDest /*= nullptr*/,
                            const RECT *pSrc /*= nullptr*/,
                            DWORD dwRop /*= SRCCOPY*/)
{
    if (!IsNull() && hDestDc)
    {
        return !!::BitBlt(hDestDc,
                          (pDest ? pDest->left : 0),
                          (pDest ? pDest->top : 0),
                          (pDest ? pDest->right - pDest->left : m_bitmapSize.cx),
                          (pDest ? pDest->bottom - pDest->top : m_bitmapSize.cy),
                          m_hDC,
                          (pSrc ? pSrc->left : 0),
                          (pSrc ? pSrc->top : 0),
                          dwRop);
    }
    else
    {
        return false;
    }
}

bool AlphaMemDc::StretchBltDraw(HDC hDestDc,
                                const RECT *pDest /*= nullptr*/,
                                const RECT *pSrc /*= nullptr*/,
                                DWORD dwRop /*= SRCCOPY*/)
{
    if (!IsNull() && hDestDc)
    {
        return !!::StretchBlt(hDestDc,
                              (pDest ? pDest->left : 0),
                              (pDest ? pDest->top : 0),
                              (pDest ? pDest->right - pDest->left : m_bitmapSize.cx),
                              (pDest ? pDest->bottom - pDest->top : m_bitmapSize.cy),
                              m_hDC,
                              (pSrc ? pSrc->left : 0),
                              (pSrc ? pSrc->top : 0),
                              (pSrc ? pSrc->right - pSrc->left : m_bitmapSize.cx),
                              (pSrc ? pSrc->bottom - pSrc->top : m_bitmapSize.cy),
                              dwRop);
    }
    else
    {
        return false;
    }
}

bool AlphaMemDc::UpdateLayeredWindowDraw(HWND hLayeredWnd, POINT pt, uint8_t alpha /*= 255*/)
{
    if (!IsNull() && ::IsWindow(hLayeredWnd))
    {
        POINT ptSrc{0};
        BLENDFUNCTION blend = {0};
        blend.BlendOp = AC_SRC_OVER;
        blend.BlendFlags = 0;
        blend.SourceConstantAlpha = alpha;
        blend.AlphaFormat = AC_SRC_ALPHA;
        return !!::UpdateLayeredWindow(
            hLayeredWnd, nullptr, &pt, &m_bitmapSize, m_hDC, &ptSrc, 0, &blend, ULW_ALPHA);
    }
    return false;
}

HBITMAP AlphaMemDc::CreateAlphaBitmap(HDC hDc,
                                      int32_t nWidth,
                                      int32_t nHeight,
                                      void **ppBitmapData /* = nullptr*/)
{
    BITMAPINFO bitmapInfo;
    std::memset(&bitmapInfo, 0, sizeof(bitmapInfo));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = std::abs(nWidth);
    bitmapInfo.bmiHeader.biHeight = -std::abs(nHeight); // top-down image
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    return ::CreateDIBSection(hDc, &bitmapInfo, DIB_RGB_COLORS, ppBitmapData, nullptr, 0);
}

SHARELIB_END_NAMESPACE
