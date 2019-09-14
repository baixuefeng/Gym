#include "targetver.h"
#include "UI/Utility/GdiplusUtility.h"
#include <cassert>
#include <cmath>
#include <string>
#include <cstring>
#include <algorithm>
#include <memory>
#include <mutex>
#include <atlbase.h>
#include <atlstr.h>

#include <ObjIdl.h>
#pragma warning(disable:4458)
#include <GdiPlus.h>
#pragma warning(default:4458)
//使用Gdiplus相关的函数需要引入的库
#pragma comment(lib, "Gdiplus.lib")

namespace
{
    class GdiplusInit
    {
    public:
        GdiplusInit()
        {
            Gdiplus::GdiplusStartupInput input;
            Gdiplus::GdiplusStartupOutput output;
            m_bInitOk = (Gdiplus::Status::Ok == Gdiplus::GdiplusStartup(&m_token, &input, &output));
        }
        ~GdiplusInit()
        {
            if (m_bInitOk)
            {
                Gdiplus::GdiplusShutdown(m_token);
                m_token = 0;
            }
        }

        bool m_bInitOk;
        ULONG_PTR m_token;
    };

}

SHARELIB_BEGIN_NAMESPACE

bool InitializeGdiplus()
{
    static bool s_bInitOk = false;
    static std::once_flag s_initOnceFlags;
    if (!s_bInitOk)
    {
        std::call_once(
            s_initOnceFlags, 
            []()
        {
            static GdiplusInit s_init;
            s_bInitOk = s_init.m_bInitOk;
        });
    }
    return s_bInitOk;
}

GuardGdipState::GuardGdipState(Gdiplus::Graphics & g)
    : m_g(g)
    , m_state(g.Save())
{
}

GuardGdipState::~GuardGdipState()
{
    m_g.Restore(m_state);
}

bool GdiplusAlphaDraw(Gdiplus::Graphics& g, const Gdiplus::RectF& destRect, Gdiplus::Bitmap& image, const Gdiplus::RectF& srcRect, uint8_t alpha/* = 255*/)
{
    Gdiplus::ColorMatrix clrMatrix =
    {
        {
            { 1, 0, 0, 0, 0 },
            { 0, 1, 0, 0, 0 },
            { 0, 0, 1, 0, 0 },
            { 0, 0, 0, Gdiplus::REAL(alpha) / 255, 0 },
            { 0, 0, 0, 0, 1 }
        }
    };
    Gdiplus::ImageAttributes imageAttr;
    imageAttr.SetColorMatrix(&clrMatrix);
    return (Gdiplus::Status::Ok == g.DrawImage(&image,
        destRect,
        srcRect.X,
        srcRect.Y,
        srcRect.Width,
        srcRect.Height,
        Gdiplus::Unit::UnitPixel,
        &imageAttr));
}

bool GetEncoderClsid(const wchar_t * pTypeName, CLSID & clsid)
{
    if (!pTypeName)
    {
        return false;
    }
    UINT nNum = 0, nSize = 0;
    if (Gdiplus::GetImageDecodersSize(&nNum, &nSize) != Gdiplus::Ok)
    {
        return false;
    }
    std::unique_ptr<uint8_t[]> spBuffer(new (std::nothrow) uint8_t[nSize]{});
    if (!spBuffer)
    {
        return false;
    }
    std::memset(spBuffer.get(), 0, nSize);
    Gdiplus::ImageCodecInfo* pCodecInfo = (Gdiplus::ImageCodecInfo*)spBuffer.get();
    if (Gdiplus::GetImageDecoders(nNum, nSize, pCodecInfo) != Gdiplus::Ok)
    {
        return false;
    }

    ATL::CString name = pTypeName;
    name.MakeUpper();
    for (UINT i = 0; i < nNum; ++i)
    {
        if (std::wcsstr(pCodecInfo[i].FilenameExtension, name))
        {
            clsid = pCodecInfo[i].Clsid;
            return true;
        }
    }
    return false;
}

void AdjustPointF(const Gdiplus::Font& font, const Gdiplus::StringFormat& strFormat, Gdiplus::PointF& pt)
{
    assert(font.IsAvailable());
    switch (strFormat.GetLineAlignment())
    {
    case Gdiplus::StringAlignment::StringAlignmentNear:
        break;
    case Gdiplus::StringAlignment::StringAlignmentCenter:
        pt.Y += GetFontBottomSpace(font) / 2;
        break;
    case Gdiplus::StringAlignment::StringAlignmentFar:
        pt.Y += GetFontBottomSpace(font);
        break;
    default:
        break;
    }
    if (font.GetStyle() & Gdiplus::FontStyle::FontStyleItalic)
    {
        switch (strFormat.GetAlignment())
        {
        case Gdiplus::StringAlignment::StringAlignmentCenter:
            pt.X -= GetFontItalicWidth(font) / 2;
            break;
        case Gdiplus::StringAlignment::StringAlignmentFar:
            pt.X -= GetFontItalicWidth(font);
            break;
        default:
            break;
        }
    }
}

void AdjustRectF(const Gdiplus::Font& font, const Gdiplus::StringFormat& strFormat, Gdiplus::RectF& rect)
{
    assert(font.IsAvailable());
    assert(font.IsAvailable());
    float realHeight = GetFontRealHeight(font);
    float needHeight = GetFontNeedHeight(font);
    if (rect.Height < realHeight)
    {
        return;
    }
    else if (rect.Height < needHeight)
    {
        switch (strFormat.GetLineAlignment())
        {
        case Gdiplus::StringAlignment::StringAlignmentCenter:
            rect.Y += (rect.Height - realHeight) / 2;
            break;
        case Gdiplus::StringAlignment::StringAlignmentFar:
            rect.Y += rect.Height - realHeight;
            break;
        default:
            break;
        }
    }
    else
    {
        switch (strFormat.GetLineAlignment())
        {
        case Gdiplus::StringAlignment::StringAlignmentNear:
            break;
        case Gdiplus::StringAlignment::StringAlignmentCenter:
            rect.Y += GetFontBottomSpace(font) / 2;
            break;
        case Gdiplus::StringAlignment::StringAlignmentFar:
            rect.Y += GetFontBottomSpace(font);
            break;
        default:
            break;
        }
    }
    if (font.GetStyle() & Gdiplus::FontStyle::FontStyleItalic)
    {
        switch (strFormat.GetAlignment())
        {
        case Gdiplus::StringAlignment::StringAlignmentCenter:
            rect.X -= GetFontItalicWidth(font) / 2;
            break;
        case Gdiplus::StringAlignment::StringAlignmentFar:
            rect.X -= GetFontItalicWidth(font);
            break;
        default:
            break;
        }
    }
    rect.Height = (std::max)(rect.Height, needHeight);
}

//----辅助函数----------------------------------------

float GetFontBottomSpace(const Gdiplus::Font& font)
{
    assert(font.IsAvailable());
    Gdiplus::FontFamily ff;
    if (Gdiplus::Status::Ok != font.GetFamily(&ff))
    {
        return 0;
    }

    auto iFontStyle = font.GetStyle();
    float emSize = ff.GetEmHeight(iFontStyle);
    float realHeight = (float)ff.GetCellAscent(iFontStyle) + (float)ff.GetCellDescent(iFontStyle);
    float needHeight = (float)ff.GetLineSpacing(iFontStyle);
    return std::abs(font.GetSize()) * (needHeight - realHeight) / emSize;
}

float GetFontRealHeight(const Gdiplus::Font& font)
{
    assert(font.IsAvailable());
    Gdiplus::FontFamily ff;
    if (Gdiplus::Status::Ok != font.GetFamily(&ff))
    {
        return 0;
    }

    auto iFontStyle = font.GetStyle();
    float emSize = ff.GetEmHeight(iFontStyle);
    float realHeight = (float)ff.GetCellAscent(iFontStyle) + (float)ff.GetCellDescent(iFontStyle);
    return std::abs(font.GetSize()) * realHeight / emSize;
}

float GetFontNeedHeight(const Gdiplus::Font& font)
{
    assert(font.IsAvailable());
    Gdiplus::FontFamily ff;
    if (Gdiplus::Status::Ok != font.GetFamily(&ff))
    {
        return 0;
    }

    float emSize = ff.GetEmHeight(font.GetStyle());
    float needHeight = (float)ff.GetLineSpacing(font.GetStyle());
    //这是gdi+完整绘制文字所需要的最小高度，向上取整
    return std::ceil(std::abs(font.GetSize()) * needHeight / emSize);
}

float GetFontItalicWidth(const Gdiplus::Font& font)
{
    assert(font.IsAvailable());
    if (font.GetStyle() & Gdiplus::FontStyle::FontStyleItalic)
    {
        return std::abs(font.GetSize()) / 6;
    }
    else
    {
        return 0;
    }
}

SHARELIB_END_NAMESPACE
