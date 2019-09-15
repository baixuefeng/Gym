#pragma once
#include <string>
#include <vector>
#include <atlcomcli.h>
#include <dwrite.h>
#include "MacroDefBase.h"
#include "UI/GraphicLayer/Layers/GraphicLayerTypeDef.h"

SHARELIB_BEGIN_NAMESPACE

/* 字体资源信息
*/
struct TFontResInfo
{
    //字体资源id
    uint32_t m_nResID = 0;

    //字体资源的类型名称
    const wchar_t *m_pResType = nullptr;

    //字体资源所在模块，null的话取当前模块
    HMODULE m_hModule = nullptr;
};

//------------------------------------------------------------------------------------------

/* 文本格式属性
*/
struct TDWriteTextAttributes
{
    // 字体集, nullptr表示使用系统字体集
    IDWriteFontCollection *m_pFontCollection = nullptr;

    // 字体名称
    const wchar_t *m_pFontName = nullptr;

    // 字体大小
    FLOAT m_fontSize = 12.f;

    // 字体Weight
    DWRITE_FONT_WEIGHT m_fontWeight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;

    // 字体风格
    DWRITE_FONT_STYLE m_fontStyle = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;

    // 字体拉伸
    DWRITE_FONT_STRETCH m_fontStretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;

    // 字体本地化
    const wchar_t *m_pLocalName = L"zh-cn";

    // 文本水平对齐方式
    DWRITE_TEXT_ALIGNMENT m_textAlignment = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;

    // 文本垂直对齐方式
    DWRITE_PARAGRAPH_ALIGNMENT m_textParagraphAlignment =
        DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;

    // 文本wrapping
    DWRITE_WORD_WRAPPING m_textWrapping = DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_WRAP;

    // 文本截尾
    DWRITE_TRIMMING m_textTrimming =
        DWRITE_TRIMMING{DWRITE_TRIMMING_GRANULARITY::DWRITE_TRIMMING_GRANULARITY_NONE, 0, 0};

    // 截断时显示内容
    IDWriteInlineObject *m_pTrimmingInlineObject = nullptr;
};

//----------------------------------------------------------------------

class DWriteUtility
{
public:
    DWriteUtility();
    ~DWriteUtility();

    /* 创建DWrite工厂
    */
    bool CreateDWriteFactory();

    /** 创建自定义字体集
    @param[in] fontInfo 字体资源信息
    @param[out] ppCustomCollection 自定义字体集
    */
    HRESULT CreateCustomDWriteFontCollection(const std::vector<TFontResInfo> &fontInfo,
                                             IDWriteFontCollection **ppCustomCollection);

    /** 创建自定义字体集
    @param[in] fontInfo 字体资源信息
    @param[out] ppCustomCollection 自定义字体集
    */
    HRESULT CreateCustomDWriteFontCollection(std::initializer_list<TFontResInfo> fontList,
                                             IDWriteFontCollection **ppCustomCollection);

    /** 创建自定义字体集
    @param[in] fontInfo 字体文件路径
    @param[out] ppCustomCollection 自定义字体集
    */
    HRESULT CreateCustomDWriteFontCollection(const std::vector<std::wstring> &fontPath,
                                             IDWriteFontCollection **ppCustomCollection);

    /** 创建自定义字体集
    @param[in] fontInfo 字体文件路径
    @param[out] ppCustomCollection 自定义字体集
    */
    HRESULT CreateCustomDWriteFontCollection(std::initializer_list<const wchar_t *> fontList,
                                             IDWriteFontCollection **ppCustomCollection);

    /** 创建文本格式
    @param[in] textAttr 文本属性
    */
    ATL::CComPtr<IDWriteTextFormat> CreateTextFormat(const TDWriteTextAttributes &textAttr);

    /** 绘制到位图
    @param[in] pLayout DWrite的文本Layout
    @param[in] szBitmap 位图大小
    @param[in] transformMx 当前转换矩阵
    @param[in] pt 绘制起点
    @param[in] textColor 文本颜色
    @return 位图指针，32位，从低到高bgra
    */
    uint8_t *DrawToBitmap(IDWriteTextLayout *pLayout,
                          SIZE szBitmap,
                          const DWRITE_MATRIX &transformMx,
                          POINT pt,
                          COLOR32 textColor);

    /** 从TextFormat获取字体信息
    @param[in] spTextFormat 字体格式
    @return 字体信息
    */
    static ATL::CComPtr<IDWriteFont> GetFontFromTextFormat(
        ATL::CComPtr<IDWriteTextFormat> spTextFormat);

    /* DWrite工厂
    */
    ATL::CComPtr<IDWriteFactory> m_spDWriteFactory;

private:
    /** 创建自定义字体集实现
    @param[in] spCollectionLoader 字体加载器
    @param[out] ppCustomCollection 自定义字体集
    */
    HRESULT CreateCustomDWriteFontCollectionImpl(
        ATL::CComPtr<IDWriteFontCollectionLoader> spCollectionLoader,
        IDWriteFontCollection **ppCustomCollection);

    class DWriteCustomRender;
    DWriteCustomRender *m_pDWriteCustomRender;
};

SHARELIB_END_NAMESPACE
