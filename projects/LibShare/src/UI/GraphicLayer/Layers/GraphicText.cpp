#include "targetver.h"
#include "UI/GraphicLayer/Layers/GraphicText.h"
#include <cmath>
#include "UI/GraphicLayer/ConfigEngine/XmlAttributeUtility.h"
#include "UI/GraphicLayer/Layers/GraphicRootLayer.h"
#pragma comment(lib, "DWrite.lib")

SHARELIB_BEGIN_NAMESPACE

IMPLEMENT_RUNTIME_DYNAMIC_CREATE(GraphicText, GraphicLayer::RegisterLayerClassesInfo)

GraphicText::GraphicText()
    : m_nDebugLevel(0)
    , m_textColor(COLOR32_ARGB(255, 0, 0, 0))
    , m_fontName(L"微软雅黑")
    , m_fontSize(12.f)
    , m_fontWeight(DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL)
    , m_fontStyle(DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL)
    , m_fontStretch(DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL)
    , m_bClearType(true)
{}

void GraphicText::OnReadingXmlAttribute(const pugi::xml_attribute &attr)
{
    if (std::wcscmp(L"lrDebugLevel", attr.name()) == 0) {
        m_nDebugLevel = attr.as_int(0);
    } else if (std::wcscmp(L"lrTextColor", attr.name()) == 0) {
        XmlAttributeUtility::ReadXmlValueColor(attr.as_string(), m_textColor);
    } else if (std::wcscmp(L"lrTextMargin", attr.name()) == 0) {
        XmlAttributeUtility::ReadXmlValue(attr.as_string(), m_textMargin);
    } else if (std::wcscmp(L"lrFont", attr.name()) == 0) {
        m_fontName = attr.as_string(L"微软雅黑");
    } else if (std::wcscmp(L"lrFontSize", attr.name()) == 0) {
        m_fontSize = attr.as_float(12.f);
    } else if (std::wcscmp(L"lrFontWeight", attr.name()) == 0) {
        m_fontWeight = (DWRITE_FONT_WEIGHT)attr.as_int(
            DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL);
    } else if (std::wcscmp(L"lrFontStyle", attr.name()) == 0) {
        m_fontStyle = (DWRITE_FONT_STYLE)attr.as_int(DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL);
    } else if (std::wcscmp(L"lrFontStretch", attr.name()) == 0) {
        m_fontStretch = (DWRITE_FONT_STRETCH)attr.as_int(
            DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL);
    } else if (std::wcscmp(L"lrClearType", attr.name()) == 0) {
        m_bClearType = attr.as_bool(true);
    } else {
        __super::OnReadingXmlAttribute(attr);
    }
}

bool GraphicText::OnReadXmlAttributeEnded(const pugi::xml_node &node)
{
    const wchar_t *pText = node.child_value();
    if (pText && *pText) {
        m_text = pText;
        CreateTextLayout();
    }
    return true;
}

void GraphicText::OnWritingAttributeToXml(pugi::xml_node node)
{
    auto attr = node.append_attribute(L"lrDebugLevel");
    attr.set_value(m_nDebugLevel);

    attr = node.append_attribute(L"lrTextColor");
    XmlAttributeUtility::WriteValueColorToXml(attr, m_textColor);

    attr = node.append_attribute(L"lrTextMargin");
    XmlAttributeUtility::WriteValueToXml(attr, m_textMargin);

    attr = node.append_attribute(L"lrFont");
    attr.set_value(m_fontName.c_str());

    attr = node.append_attribute(L"lrFontSize");
    attr.set_value(m_fontSize);

    attr = node.append_attribute(L"lrFontWeight");
    attr.set_value(m_fontWeight);

    attr = node.append_attribute(L"lrFontStyle");
    attr.set_value(m_fontStyle);

    attr = node.append_attribute(L"lrFontStretch");
    attr.set_value(m_fontStretch);

    attr = node.append_attribute(L"lrClearType");
    attr.set_value(m_bClearType);

    auto nodeText = node.append_child(pugi::xml_node_type::node_cdata);
    nodeText.set_value(m_text.c_str());

    __super::OnWritingAttributeToXml(node);
}

bool GraphicText::CreateTextLayout()
{
    if (m_text.empty()) {
        return true;
    }
    auto pRoot = GetRootGraphicLayer();
    if (!pRoot || !pRoot->DWritePack().m_spDWriteFactory) {
        return false;
    }
    ATL::CComPtr<IDWriteTextFormat> spFormat;
    HRESULT hr = pRoot->DWritePack().m_spDWriteFactory->CreateTextFormat(m_fontName.c_str(),
                                                                         NULL,
                                                                         m_fontWeight,
                                                                         m_fontStyle,
                                                                         m_fontStretch,
                                                                         m_fontSize,
                                                                         L"zh-cn",
                                                                         &spFormat);
    if (FAILED(hr)) {
        return false;
    }

    ATL::CComPtr<IDWriteTextLayout> spTextLayout;
    CD2DSizeF sz{FLT_MAX, FLT_MAX};
    if (!GetLayerBounds().IsRectEmpty()) {
        sz.width = (FLOAT)GetLayerBounds().Width() - m_textMargin.left - m_textMargin.right;
        sz.height = (FLOAT)GetLayerBounds().Height() - m_textMargin.top - m_textMargin.bottom;
    }
    hr = pRoot->DWritePack().m_spDWriteFactory->CreateTextLayout(
        m_text.c_str(), (UINT32)m_text.size(), spFormat, sz.width, sz.height, &spTextLayout);
    if (FAILED(hr)) {
        return false;
    }
    m_spTextLayout = spTextLayout;
    if (GetLayerBounds().IsRectEmpty()) {
        DWRITE_TEXT_METRICS dwMx{};
        m_spTextLayout->GetMetrics(&dwMx);
        CRect rcBound = GetLayerBounds();
        CD2DSizeF szExtra(m_textMargin.left + m_textMargin.right,
                          m_textMargin.top + m_textMargin.bottom);
        if (m_fontStyle != DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL) {
            //斜体字,宽度增加一些,否则文字会被裁掉一部分
            szExtra.width += m_fontSize / 6;
        }
        rcBound.right = rcBound.left +
                        (LONG)std::ceil(dwMx.widthIncludingTrailingWhitespace + szExtra.width);
        rcBound.bottom = rcBound.top + (LONG)std::ceil(dwMx.height + szExtra.height);
        SetLayerBounds(rcBound, true);
    }
    return true;
}

void GraphicText::Paint(D2DRenderPack &d2dRender)
{
    if (m_spTextLayout && COLOR32_GetA(m_textColor)) {
        d2dRender.m_spSolidBrush->SetColor(
            D2D1::ColorF(m_textColor, COLOR32_GetA(m_textColor) / 255.f));

        auto oldTextMode = d2dRender.m_spD2DRender->GetTextAntialiasMode();
        d2dRender.m_spD2DRender->SetTextAntialiasMode(
            m_bClearType ? D2D1_TEXT_ANTIALIAS_MODE::D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE
                         : D2D1_TEXT_ANTIALIAS_MODE::D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        d2dRender.m_spD2DRender->DrawTextLayout(
            m_textMargin.TopLeft(),
            m_spTextLayout,
            d2dRender.m_spSolidBrush,
            D2D1_DRAW_TEXT_OPTIONS::D2D1_DRAW_TEXT_OPTIONS_CLIP);
        d2dRender.m_spD2DRender->SetTextAntialiasMode(oldTextMode);

        if (m_nDebugLevel > 0 && !m_text.empty()) {
            auto oldAntialiasMode = d2dRender.m_spD2DRender->GetAntialiasMode();
            d2dRender.m_spD2DRender->SetAntialiasMode(
                D2D1_ANTIALIAS_MODE::D2D1_ANTIALIAS_MODE_ALIASED);
            d2dRender.m_spSolidBrush->SetColor(D2D1::ColorF(1.0f, 0, 0));

            DWRITE_TEXT_METRICS textMx{};
            {
                m_spTextLayout->GetMetrics(&textMx);
                CD2DRectF textRect(0, 0, textMx.widthIncludingTrailingWhitespace, textMx.height);
                textRect.OffsetRect(m_textMargin.TopLeft());
                d2dRender.m_spD2DRender->DrawRectangle(textRect, d2dRender.m_spSolidBrush);
            }

            std::vector<DWRITE_LINE_METRICS> lineMxs;
            {
                lineMxs.assign(textMx.lineCount, DWRITE_LINE_METRICS{0});
                UINT32 nLineCount = 0;
                m_spTextLayout->GetLineMetrics(&lineMxs[0], textMx.lineCount, &nLineCount);
                assert(nLineCount == textMx.lineCount);
            }

            std::vector<DWRITE_CLUSTER_METRICS> clusterMxs;
            size_t nClusterIndex = 0;
            if (m_nDebugLevel > 1) {
                UINT32 nClusterCount = 0;
                m_spTextLayout->GetClusterMetrics(nullptr, 0, &nClusterCount);
                clusterMxs.assign(nClusterCount, DWRITE_CLUSTER_METRICS{});
                m_spTextLayout->GetClusterMetrics(&clusterMxs[0], nClusterCount, &nClusterCount);
            }

            CD2DPointF ptLineStart(m_textMargin.left, m_textMargin.top);
            CD2DPointF ptLineEnd(m_textMargin.left + textMx.widthIncludingTrailingWhitespace,
                                 m_textMargin.top);
            CD2DPointF ptBaseLine(0, 0);
            for (size_t i = 0; i < lineMxs.size(); ++i) {
                ptBaseLine.y = lineMxs[i].baseline;
                d2dRender.m_spSolidBrush->SetColor(D2D1::ColorF(0, 0, 1.0f));
                d2dRender.m_spD2DRender->DrawLine(
                    ptLineStart + ptBaseLine, ptLineEnd + ptBaseLine, d2dRender.m_spSolidBrush);

                if (m_nDebugLevel > 1) {
                    CD2DPointF ptClusterStart(ptLineStart.x, ptLineStart.y);
                    CD2DPointF ptClusterEnd(ptLineStart.x, ptLineStart.y + lineMxs[i].height);
                    d2dRender.m_spSolidBrush->SetColor(D2D1::ColorF(1.0f, 0, 0));
                    for (UINT32 j = 0; j < lineMxs[i].length;
                         j += clusterMxs[nClusterIndex++].length) {
                        //此处不能用++j进行遍历,如"😄"之类的特殊字符,占用两个wchar_t, 总共4个字节, 因此
                        //一个clusterMxs中的length可能大于1
                        ptClusterStart.x += clusterMxs[nClusterIndex].width;
                        ptClusterEnd.x += clusterMxs[nClusterIndex].width;
                        d2dRender.m_spD2DRender->DrawLine(
                            ptClusterStart, ptClusterEnd, d2dRender.m_spSolidBrush);
                    }
                }

                ptLineStart.y += lineMxs[i].height;
                ptLineEnd.y += lineMxs[i].height;
                d2dRender.m_spSolidBrush->SetColor(D2D1::ColorF(1.0f, 0, 0));
                if (i + 1 < lineMxs.size()) {
                    d2dRender.m_spD2DRender->DrawLine(
                        ptLineStart, ptLineEnd, d2dRender.m_spSolidBrush);
                }
            }

            d2dRender.m_spD2DRender->SetAntialiasMode(oldAntialiasMode);
        }
    }
}

SHARELIB_END_NAMESPACE
