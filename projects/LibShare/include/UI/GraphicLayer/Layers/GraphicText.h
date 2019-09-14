#pragma once
#include "MacroDefBase.h"
#include "UI/GraphicLayer/Layers/GraphicLayer.h"
#include <string>
#include <dwrite.h>
#include <atlbase.h>
#include <atlcom.h>

SHARELIB_BEGIN_NAMESPACE

class GraphicText :
    public GraphicLayer
{
    DECLARE_RUNTIME_DYNAMIC_CREATE(GraphicText, L"text")

public:
    GraphicText();

/*
子结点的pcdata或者cdata类型的文本，即作为该文本Layer的文本内容，如果没有设置Layer的Bounds，使用文本大小加空白的大小作为Bounds
"lrDebugLevel"  : <0,1,2><int><调试文本的等级，0:不调试, 1:按行绘制框线, 2:按字绘制框线><0>
"lrTextColor"   : <16进制颜色><COLOR32><文本颜色><FF000000>
"lrTextMargin"  : <left,top,right,botton><float><文本空白，用来调整文本的显示位置><0,0,0,0>
"lrFont"        : <文本><string><字体名称><L"微软雅黑">
"lrFontSize"    : <浮点值><float><字体大小，用正值><12.0>
"lrFontWeight"  : <数值><uint32_t><字体重量，100~900，100的整倍数><400>
"lrFontStyle"   : <数值><uint32_t><字体风格，0:Normal，1:Oblique, 2:Italic><0>
"lrFontStretch" : <数值><uint32_t>
                  <字体拉伸，0:Not known，1:Ultra-condensed, 2:Extra-condensed, 3:Condensed, 4:Semi-condensed, 
                  5:Normal, 6:Semi-expanded, 7:Expanded, 8:Extra-expanded, 9:Ultra-expanded>
                  <5>
"lrClearType"   : <0或1><bool>
                  <cleartype还是grayscale,如果在层窗口上并且没有背景,必须用grayscale,否则文字透掉,除此之外,用cleartype
                  绘制效果更好>
                  <true>
*/
    virtual void OnReadingXmlAttribute(const pugi::xml_attribute & attr) override;
    virtual bool OnReadXmlAttributeEnded(const pugi::xml_node & node) override;
    virtual void OnWritingAttributeToXml(pugi::xml_node node) override;

protected:
    bool CreateTextLayout();

    virtual void Paint(D2DRenderPack & d2dRender) override;

protected:
    int m_nDebugLevel;
    ATL::CComPtr<IDWriteTextLayout> m_spTextLayout;
    COLOR32 m_textColor;
    CD2DRectF m_textMargin;
    std::wstring m_text;
    std::wstring m_fontName;
    FLOAT m_fontSize;
    DWRITE_FONT_WEIGHT m_fontWeight;
    DWRITE_FONT_STYLE m_fontStyle;
    DWRITE_FONT_STRETCH m_fontStretch;
    bool m_bClearType;
};

SHARELIB_END_NAMESPACE
