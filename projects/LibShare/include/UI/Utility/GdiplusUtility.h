#pragma once
#include "MacroDefBase.h"
#include <cstdint>
#include <Guiddef.h>

namespace Gdiplus
{
    class StringFormat;
    class Font;
    class PointF;
    class RectF;
    class Bitmap;
    class Graphics;
}

SHARELIB_BEGIN_NAMESPACE

/** 初始化Gdiplus库
*/
bool InitializeGdiplus();

class GuardGdipState
{
    SHARELIB_DISABLE_COPY_CLASS(GuardGdipState);
public:
    explicit GuardGdipState(Gdiplus::Graphics & g);
    ~GuardGdipState();

private:
    Gdiplus::Graphics & m_g;
    unsigned m_state;
};

/** 封装Gdiplus::Graphics::DrawImage, 比较耗时(15毫秒左右)
*/
bool GdiplusAlphaDraw(Gdiplus::Graphics& g, const Gdiplus::RectF& destRect, Gdiplus::Bitmap& image, const Gdiplus::RectF& srcRect, uint8_t alpha = 255);

/** 获取图片缟码器的CLSID
@param[in] pTypeName 图片类型名称,必须使用大写,如L"PNG",L"GIF"
@param[out] clsid 图片编码器的CLSID
*/
bool GetEncoderClsid(const wchar_t * pTypeName, CLSID & clsid);

/** 调整点坐标，以使文字精确对齐
*/
void AdjustPointF(const Gdiplus::Font& font, const Gdiplus::StringFormat& strFormat, Gdiplus::PointF& pt);

/** 调整矩形区域，以使文字精确对齐
*/
void AdjustRectF(const Gdiplus::Font& font, const Gdiplus::StringFormat& strFormat, Gdiplus::RectF& rect);

//----辅助函数----------------------------------------------------------------

/** 获取字体底部空白（衬线）大小
*/
float GetFontBottomSpace(const Gdiplus::Font& font);

/** 获取字体实际高度（不包含衬线）
*/
float GetFontRealHeight(const Gdiplus::Font& font);

/** 获取字体完整绘制所需要的高度（包含衬线）,向上取整
*/
float GetFontNeedHeight(const Gdiplus::Font& font);

/** 获取由于字体倾斜需要添加的宽度值(Typographic方式的字体格式,绘制斜体文字时右边会被截断, 因此需要修正宽度)
*/
float GetFontItalicWidth(const Gdiplus::Font& font);

SHARELIB_END_NAMESPACE
