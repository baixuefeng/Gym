#include "targetver.h"
#include "UI/DirectX/DWriteUtility.h"
#include <cassert>
#include <cmath>
#include <memory>
#include <mutex>
#include <atlbase.h>
#include <atlcom.h>
#include <atlfile.h>
#include <atltypes.h>

SHARELIB_BEGIN_NAMESPACE

template<class ImplClass>
using TNoLockComObject = ATL::CComObjectNoLock<ImplClass>;

//----------------------------------------------------------------------------------

/** 辅助类，内存字体加载器
*/
class CustomMemoryFontLoader
    : public IDWriteFontFileLoader
    , public IDWriteFontFileStream
    , public ATL::CComObjectRoot
{
    BEGIN_COM_MAP(CustomMemoryFontLoader)
    COM_INTERFACE_ENTRY(IDWriteFontFileLoader)
    COM_INTERFACE_ENTRY(IDWriteFontFileStream)
    END_COM_MAP()

public:
    CustomMemoryFontLoader()
        : m_pRes(nullptr)
        , m_nResSize(0)
    {}

    ~CustomMemoryFontLoader() {}

    /** 初始化加载器
    @param[in] pRes 内存字体指针
    @param[in] nSize 内存大小
    @return 初始化是否成功
    */
    bool Initialize(const void *pRes, uint32_t nSize)
    {
        if (!pRes || (nSize == 0)) {
            return false;
        }
        m_pRes = pRes;
        m_nResSize = nSize;
        return true;
    }

public:
    //----IDWriteFontFileLoader--------------------------------------------

    STDMETHOD(CreateStreamFromKey)
    (void const * /*fontFileReferenceKey*/,
     UINT32 /*fontFileReferenceKeySize*/,
     _COM_Outptr_ IDWriteFontFileStream **fontFileStream)
    {
        if (SUCCEEDED(QueryInterface(__uuidof(IDWriteFontFileStream), (void **)fontFileStream))) {
            return S_OK;
        }
        return E_FAIL;
    }

    //----IDWriteFontFileStream-------------------------------------------

    STDMETHOD(ReadFileFragment)
    (_Outptr_result_bytebuffer_(fragmentSize) void const **fragmentStart,
     UINT64 fileOffset,
     UINT64 fragmentSize,
     _Out_ void **fragmentContext)
    {
        if ((fileOffset <= m_nResSize) && (fragmentSize <= m_nResSize - fileOffset)) {
            *fragmentStart = static_cast<uint8_t const *>(m_pRes) + fileOffset;
            *fragmentContext = NULL;
            return S_OK;
        } else {
            *fragmentStart = NULL;
            *fragmentContext = NULL;
            return E_FAIL;
        }
    }

    STDMETHOD_(void, ReleaseFileFragment)(void * /*fragmentContext*/) {}

    STDMETHOD(GetFileSize)(_Out_ UINT64 *fileSize)
    {
        if (fileSize) {
            *fileSize = m_nResSize;
            return S_OK;
        }
        return E_INVALIDARG;
    }

    STDMETHOD(GetLastWriteTime)(_Out_ UINT64 *lastWriteTime)
    {
        if (lastWriteTime) {
            *lastWriteTime = 0;
        }
        return E_NOTIMPL;
    }

private:
    const void *m_pRes;
    uint32_t m_nResSize;
};

//-----------------------------------------------------------------------------------

class CustomCollectionLoader
    : public IDWriteFontCollectionLoader
    , public IDWriteFontFileEnumerator
    , public ATL::CComObjectRoot
{
    BEGIN_COM_MAP(CustomCollectionLoader)
    COM_INTERFACE_ENTRY(IDWriteFontCollectionLoader)
    COM_INTERFACE_ENTRY(IDWriteFontFileEnumerator)
    END_COM_MAP()

public:
    struct FontMemInfo
    {
        const void *m_pRes = nullptr; //内存指针
        uint32_t m_nSize = 0;         //内存大小
    };

    static bool GetResInfo(const TFontResInfo &info, FontMemInfo &fontMem)
    {
        if ((info.m_nResID == 0) || (info.m_pResType == nullptr)) {
            return false;
        }
        HMODULE hModule = (info.m_hModule == nullptr ? (HMODULE)&__ImageBase : info.m_hModule);
        HRSRC hRes = ::FindResource(hModule, MAKEINTRESOURCE(info.m_nResID), info.m_pResType);
        if (hRes != nullptr) {
            HGLOBAL memHandle = ::LoadResource(hModule, hRes);
            if (memHandle != nullptr) {
                fontMem.m_pRes = ::LockResource(memHandle);
                if (fontMem.m_pRes != nullptr) {
                    fontMem.m_nSize = ::SizeofResource(hModule, hRes);
                    return true;
                }
            }
        }
        assert(!"资源加载失败");
        return false;
    }

    CustomCollectionLoader()
        : m_bLoadFromFile(false)
        , m_nIndex(0)
    {}

    ~CustomCollectionLoader() {}

    bool Initialize(IDWriteFactory *pFactory, const std::vector<TFontResInfo> &fontInfo)
    {
        if (pFactory == nullptr) {
            return false;
        }
        std::vector<FontMemInfo> fontMemArray;
        fontMemArray.reserve(fontInfo.size());
        for (auto &info : fontInfo) {
            FontMemInfo fontMem;
            if (GetResInfo(info, fontMem)) {
                fontMemArray.push_back(fontMem);
            } else {
                return false;
            }
        }
        assert(!m_spDWFactory);
        m_spDWFactory.Release();
        m_spDWFactory = pFactory;
        m_fontMemArray.swap(fontMemArray);
        m_bLoadFromFile = false;
        return true;
    }

    bool Initialize(IDWriteFactory *pFactory, const std::vector<std::wstring> &fontInfo)
    {
        if (pFactory == nullptr) {
            return false;
        }
        ATL::CAtlFile file;
        HRESULT hr = E_FAIL;
        for (auto &info : fontInfo) {
            hr = file.Create(info.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);
            if (FAILED(hr)) {
                assert(!"文件不存在");
                return false;
            }
            file.Close();
        }
        assert(!m_spDWFactory);
        m_spDWFactory.Release();
        m_spDWFactory = pFactory;
        m_fontPathArray = fontInfo;
        m_bLoadFromFile = true;
        return true;
    }

public:
    //----IDWriteFontCollectionLoader----------------------------------------------

    STDMETHOD(CreateEnumeratorFromKey)
    (IDWriteFactory * /*factory*/,
     void const * /*collectionKey*/,
     UINT32 /*collectionKeySize*/,
     _COM_Outptr_ IDWriteFontFileEnumerator **fontFileEnumerator)
    {
        if (!fontFileEnumerator) {
            return E_INVALIDARG;
        }
        *fontFileEnumerator = nullptr;
        HRESULT hr = QueryInterface(__uuidof(IDWriteFontFileEnumerator),
                                    (void **)fontFileEnumerator);
        if (SUCCEEDED(hr)) {
            return S_OK;
        }
        *fontFileEnumerator = nullptr;
        return E_FAIL;
    }

    //----IDWriteFontFileEnumerator-------------------------------------------------------------

    STDMETHOD(MoveNext)(_Out_ BOOL *hasCurrentFile)
    {
        if (hasCurrentFile) {
            size_t nSize = m_bLoadFromFile ? m_fontPathArray.size() : m_fontMemArray.size();
            if (m_nIndex < nSize) {
                *hasCurrentFile = TRUE;
                ++m_nIndex;
            } else {
                *hasCurrentFile = FALSE;
            }
        }
        return S_OK;
    }

    STDMETHOD(GetCurrentFontFile)(_COM_Outptr_ IDWriteFontFile **fontFile)
    {
        if (!fontFile) {
            return E_INVALIDARG;
        }
        if (!m_spDWFactory || (m_nIndex == 0)) {
            return E_INVALIDARG;
        }

        *fontFile = nullptr;
        HRESULT hr = E_FAIL;
        if (m_bLoadFromFile) {
            if (m_fontPathArray.empty() || (m_nIndex > m_fontPathArray.size())) {
                return E_INVALIDARG;
            }
            hr = m_spDWFactory->CreateFontFileReference(
                m_fontPathArray[m_nIndex - 1].c_str(), nullptr, fontFile);
            return hr;
        } else {
            if (m_fontMemArray.empty() || (m_nIndex > m_fontMemArray.size())) {
                return E_INVALIDARG;
            }
            auto pImpl = new TNoLockComObject<CustomMemoryFontLoader>;
            if (!pImpl->Initialize(m_fontMemArray[m_nIndex - 1].m_pRes,
                                   m_fontMemArray[m_nIndex - 1].m_nSize)) {
                delete pImpl;
                assert(!"bug");
                return E_NOTIMPL;
            }
            ATL::CComPtr<IDWriteFontFileLoader> spFontFileLoader;
            hr = pImpl->QueryInterface(__uuidof(IDWriteFontFileLoader), (void **)&spFontFileLoader);
            if (FAILED(hr)) {
                delete pImpl;
                assert(!"bug");
                return E_NOTIMPL;
            }

            hr = m_spDWFactory->RegisterFontFileLoader(spFontFileLoader);
            if (FAILED(hr)) {
                return E_INVALIDARG;
            }
            hr = m_spDWFactory->CreateCustomFontFileReference(GetResInfo, //该参数不使用
                                                              sizeof(&GetResInfo), //该参数不使用
                                                              spFontFileLoader,
                                                              fontFile);
            return hr;
        }
    }

private:
    bool m_bLoadFromFile;
    size_t m_nIndex;
    ATL::CComPtr<IDWriteFactory> m_spDWFactory;
    std::vector<FontMemInfo> m_fontMemArray;
    std::vector<std::wstring> m_fontPathArray;
};

//-----------------------------------------------------------------------------------------------

//gamma校正
static std::once_flag g_gammaInitFlags;
static uint8_t g_gammaCorrection[256];

class DWriteUtility::DWriteCustomRender
    : public IDWriteTextRenderer
    , public ATL::CComObjectRoot
{
    BEGIN_COM_MAP(DWriteCustomRender)
    COM_INTERFACE_ENTRY(IDWritePixelSnapping)
    COM_INTERFACE_ENTRY(IDWriteTextRenderer)
    END_COM_MAP()

public:
    DWriteCustomRender()
    {
        std::call_once(g_gammaInitFlags, []() {
            for (int i = 0; i < _countof(g_gammaCorrection); ++i) {
                g_gammaCorrection[i] = (uint8_t)std::round(std::pow((double)i / 255.0, 0.483) *
                                                           255.0);
            }
        });
    }

    void Resize(SIZE sz)
    {
        if (m_szBitmap != sz) {
            m_szBitmap = sz;
            m_spBitmap.reset(new uint8_t[sz.cx * sz.cy * 4]{0});
            m_spColorMap.reset(new uint8_t[sz.cx * sz.cy * 3]{0});
        } else {
            std::memset(m_spBitmap.get(), 0, m_szBitmap.cx * m_szBitmap.cy * 4);
            std::memset(m_spColorMap.get(), 0, m_szBitmap.cx * m_szBitmap.cy * 3);
        }
    }

public:
    /// <summary>
    /// Determines whether pixel snapping is disabled. The recommended default is FALSE,
    /// unless doing animation that requires subpixel vertical placement.
    /// </summary>
    /// <param name="clientDrawingContext">The context passed to IDWriteTextLayout::Draw.</param>
    /// <param name="isDisabled">Receives TRUE if pixel snapping is disabled or FALSE if it not.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(IsPixelSnappingDisabled)
    (__maybenull void *clientDrawingContext, __out BOOL *isDisabled)
    {
        UNREFERENCED_PARAMETER(clientDrawingContext);
        if (isDisabled) {
            *isDisabled = FALSE;
        }
        return S_OK;
    }

    /// <summary>
    /// Gets the current transform that maps abstract coordinates to DIPs,
    /// which may disable pixel snapping upon any rotation or shear.
    /// </summary>
    /// <param name="clientDrawingContext">The context passed to IDWriteTextLayout::Draw.</param>
    /// <param name="transform">Receives the transform.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetCurrentTransform)
    (__maybenull void *clientDrawingContext, __out DWRITE_MATRIX *transform)
    {
        UNREFERENCED_PARAMETER(clientDrawingContext);
        if (transform) {
            DWRITE_MATRIX mx;
            std::memset(&mx, 0, sizeof(mx));
            mx.m11 = 1.0f;
            mx.m22 = 1.0f;
            *transform = mx;
        }
        return S_OK;
    }

    /// <summary>
    /// Gets the number of physical pixels per DIP. A DIP (device-independent pixel) is 1/96 inch,
    /// so the pixelsPerDip value is the number of logical pixels per inch divided by 96 (yielding
    /// a value of 1 for 96 DPI and 1.25 for 120).
    /// </summary>
    /// <param name="clientDrawingContext">The context passed to IDWriteTextLayout::Draw.</param>
    /// <param name="pixelsPerDip">Receives the number of physical pixels per DIP.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetPixelsPerDip)(__maybenull void *clientDrawingContext, __out FLOAT *pixelsPerDip)
    {
        UNREFERENCED_PARAMETER(clientDrawingContext);
        if (pixelsPerDip) {
            *pixelsPerDip = 1.0f;
        }
        return S_OK;
    }

    /// <summary>
    /// IDWriteTextLayout::Draw calls this function to instruct the client to
    /// render a run of glyphs.
    /// </summary>
    /// <param name="clientDrawingContext">The context passed to
    /// IDWriteTextLayout::Draw.</param>
    /// <param name="baselineOriginX">X-coordinate of the baseline.</param>
    /// <param name="baselineOriginY">Y-coordinate of the baseline.</param>
    /// <param name="measuringMode">Specifies measuring method for glyphs in the run.
    /// Renderer implementations may choose different rendering modes for given measuring methods,
    /// but best results are seen when the rendering mode matches the corresponding measuring mode:
    /// DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL for DWRITE_MEASURING_MODE_NATURAL
    /// DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC for DWRITE_MEASURING_MODE_GDI_CLASSIC
    /// DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL for DWRITE_MEASURING_MODE_GDI_NATURAL
    /// </param>
    /// <param name="glyphRun">The glyph run to draw.</param>
    /// <param name="glyphRunDescription">Properties of the characters
    /// associated with this run.</param>
    /// <param name="clientDrawingEffect">The drawing effect set in
    /// IDWriteTextLayout::SetDrawingEffect.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(DrawGlyphRun)
    (__maybenull void *clientDrawingContext,
     FLOAT baselineOriginX,
     FLOAT baselineOriginY,
     DWRITE_MEASURING_MODE measuringMode,
     __in DWRITE_GLYPH_RUN const *glyphRun,
     __in DWRITE_GLYPH_RUN_DESCRIPTION const *glyphRunDescription,
     __maybenull IUnknown *clientDrawingEffect)
    {
        UNREFERENCED_PARAMETER(clientDrawingContext);
        UNREFERENCED_PARAMETER(clientDrawingContext);
        UNREFERENCED_PARAMETER(glyphRunDescription);
        UNREFERENCED_PARAMETER(clientDrawingEffect);
        ATL::CComPtr<IDWriteGlyphRunAnalysis> spGlyphRunAnalysis;
        assert(m_spDWriteFactory);
        HRESULT hr = m_spDWriteFactory->CreateGlyphRunAnalysis(
            glyphRun,
            1.0f,
            &m_transformMx,
            DWRITE_RENDERING_MODE::DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC,
            measuringMode,
            baselineOriginX,
            baselineOriginY,
            &spGlyphRunAnalysis);
        assert(SUCCEEDED(hr));
        if (FAILED(hr)) {
            return hr;
        }
        RECT rcBounds{0, 0, m_szBitmap.cx, m_szBitmap.cy};
        hr = spGlyphRunAnalysis->CreateAlphaTexture(
            DWRITE_TEXTURE_TYPE::DWRITE_TEXTURE_CLEARTYPE_3x1,
            &rcBounds,
            m_spColorMap.get(),
            m_szBitmap.cx * m_szBitmap.cy * 3);
        assert(SUCCEEDED(hr));
        if (FAILED(hr)) {
            return hr;
        }
        hr = spGlyphRunAnalysis->GetAlphaTextureBounds(
            DWRITE_TEXTURE_TYPE::DWRITE_TEXTURE_CLEARTYPE_3x1, &rcBounds);
        assert(SUCCEEDED(hr));
        if (FAILED(hr)) {
            return hr;
        }

        size_t nRowIndex = 0;
        uint8_t *pColorMapIndex = nullptr, *pBitmapIndex = nullptr;
        double rRate = 0, gRate = 0, bRate = 0;
        for (auto row = rcBounds.top; row < rcBounds.bottom; ++row) {
            nRowIndex = row * m_szBitmap.cx;
            for (auto column = rcBounds.left; column < rcBounds.right; ++column) {
                pColorMapIndex = m_spColorMap.get() + (nRowIndex + column) * 3;
                pBitmapIndex = m_spBitmap.get() + (nRowIndex + column) * 4;
                if (pColorMapIndex[0] || pColorMapIndex[1] || pColorMapIndex[2]) {
                    rRate = g_gammaCorrection[pColorMapIndex[0]] / 255.0;
                    gRate = g_gammaCorrection[pColorMapIndex[1]] / 255.0;
                    bRate = g_gammaCorrection[pColorMapIndex[2]] / 255.0;
                    pBitmapIndex[0] = (uint8_t)std::round(bRate * COLOR32_GetB(m_textColor));
                    pBitmapIndex[1] = (uint8_t)std::round(gRate * COLOR32_GetG(m_textColor));
                    pBitmapIndex[2] = (uint8_t)std::round(rRate * COLOR32_GetR(m_textColor));
                    pBitmapIndex[3] = (uint8_t)std::round((rRate + gRate + bRate) / 3.0 * 255.0);
                }
            }
        }
        return hr;
    }

    /// <summary>
    /// IDWriteTextLayout::Draw calls this function to instruct the client to draw
    /// an underline.
    /// </summary>
    /// <param name="clientDrawingContext">The context passed to
    /// IDWriteTextLayout::Draw.</param>
    /// <param name="baselineOriginX">X-coordinate of the baseline.</param>
    /// <param name="baselineOriginY">Y-coordinate of the baseline.</param>
    /// <param name="underline">Underline logical information.</param>
    /// <param name="clientDrawingEffect">The drawing effect set in
    /// IDWriteTextLayout::SetDrawingEffect.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// A single underline can be broken into multiple calls, depending on
    /// how the formatting changes attributes. If font sizes/styles change
    /// within an underline, the thickness and offset will be averaged
    /// weighted according to characters.
    /// To get the correct top coordinate of the underline rect, add underline::offset
    /// to the baseline's Y. Otherwise the underline will be immediately under the text.
    /// The x coordinate will always be passed as the left side, regardless
    /// of text directionality. This simplifies drawing and reduces the
    /// problem of round-off that could potentially cause gaps or a double
    /// stamped alpha blend. To avoid alpha overlap, round the end points
    /// to the nearest device pixel.
    /// </remarks>
    STDMETHOD(DrawUnderline)
    (__maybenull void *clientDrawingContext,
     FLOAT baselineOriginX,
     FLOAT baselineOriginY,
     __in DWRITE_UNDERLINE const *underline,
     __maybenull IUnknown *clientDrawingEffect)
    {
        UNREFERENCED_PARAMETER(clientDrawingContext);
        UNREFERENCED_PARAMETER(baselineOriginX);
        UNREFERENCED_PARAMETER(baselineOriginY);
        UNREFERENCED_PARAMETER(underline);
        UNREFERENCED_PARAMETER(clientDrawingEffect);
        return E_NOTIMPL;
    }

    /// <summary>
    /// IDWriteTextLayout::Draw calls this function to instruct the client to draw
    /// a strikethrough.
    /// </summary>
    /// <param name="clientDrawingContext">The context passed to
    /// IDWriteTextLayout::Draw.</param>
    /// <param name="baselineOriginX">X-coordinate of the baseline.</param>
    /// <param name="baselineOriginY">Y-coordinate of the baseline.</param>
    /// <param name="strikethrough">Strikethrough logical information.</param>
    /// <param name="clientDrawingEffect">The drawing effect set in
    /// IDWriteTextLayout::SetDrawingEffect.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// A single strikethrough can be broken into multiple calls, depending on
    /// how the formatting changes attributes. Strikethrough is not averaged
    /// across font sizes/styles changes.
    /// To get the correct top coordinate of the strikethrough rect,
    /// add strikethrough::offset to the baseline's Y.
    /// Like underlines, the x coordinate will always be passed as the left side,
    /// regardless of text directionality.
    /// </remarks>
    STDMETHOD(DrawStrikethrough)
    (__maybenull void *clientDrawingContext,
     FLOAT baselineOriginX,
     FLOAT baselineOriginY,
     __in DWRITE_STRIKETHROUGH const *strikethrough,
     __maybenull IUnknown *clientDrawingEffect)
    {
        UNREFERENCED_PARAMETER(clientDrawingContext);
        UNREFERENCED_PARAMETER(baselineOriginX);
        UNREFERENCED_PARAMETER(baselineOriginY);
        UNREFERENCED_PARAMETER(strikethrough);
        UNREFERENCED_PARAMETER(clientDrawingEffect);
        return E_NOTIMPL;
    }

    /// <summary>
    /// IDWriteTextLayout::Draw calls this application callback when it needs to
    /// draw an inline object.
    /// </summary>
    /// <param name="clientDrawingContext">The context passed to IDWriteTextLayout::Draw.</param>
    /// <param name="originX">X-coordinate at the top-left corner of the inline object.</param>
    /// <param name="originY">Y-coordinate at the top-left corner of the inline object.</param>
    /// <param name="inlineObject">The object set using IDWriteTextLayout::SetInlineObject.</param>
    /// <param name="isSideways">The object should be drawn on its side.</param>
    /// <param name="isRightToLeft">The object is in an right-to-left context and should be drawn flipped.</param>
    /// <param name="clientDrawingEffect">The drawing effect set in
    /// IDWriteTextLayout::SetDrawingEffect.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// The right-to-left flag is a hint for those cases where it would look
    /// strange for the image to be shown normally (like an arrow pointing to
    /// right to indicate a submenu).
    /// </remarks>
    STDMETHOD(DrawInlineObject)
    (__maybenull void *clientDrawingContext,
     FLOAT originX,
     FLOAT originY,
     IDWriteInlineObject *inlineObject,
     BOOL isSideways,
     BOOL isRightToLeft,
     __maybenull IUnknown *clientDrawingEffect)
    {
        UNREFERENCED_PARAMETER(clientDrawingContext);
        UNREFERENCED_PARAMETER(originX);
        UNREFERENCED_PARAMETER(originY);
        UNREFERENCED_PARAMETER(inlineObject);
        UNREFERENCED_PARAMETER(isSideways);
        UNREFERENCED_PARAMETER(isRightToLeft);
        UNREFERENCED_PARAMETER(clientDrawingEffect);
        return E_NOTIMPL;
    }

public:
    ATL::CComPtr<IDWriteFactory> m_spDWriteFactory;

    ATL::CComPtr<IDWriteRenderingParams> m_spRenderingParams;

    COLOR32 m_textColor = 0;

    CSize m_szBitmap;

    std::unique_ptr<uint8_t[]> m_spBitmap;

    std::unique_ptr<uint8_t[]> m_spColorMap;

    DWRITE_MATRIX m_transformMx;
};

//-----------------------------------------------------------------------------------------------

DWriteUtility::DWriteUtility()
    : m_pDWriteCustomRender(nullptr)
{}

DWriteUtility::~DWriteUtility()
{
    if (m_pDWriteCustomRender) {
        delete m_pDWriteCustomRender;
        m_pDWriteCustomRender = nullptr;
    }
}

bool DWriteUtility::CreateDWriteFactory()
{
    using TDWriteCreateFunc = HRESULT(WINAPI *)(
        _In_ DWRITE_FACTORY_TYPE factoryType, _In_ REFIID iid, _COM_Outptr_ IUnknown * *factory);

    static TDWriteCreateFunc s_dWriteCreateFunc = nullptr;
    static std::once_flag s_d2dInitFlags;
    if (!s_dWriteCreateFunc) {
        std::call_once(s_d2dInitFlags, [this]() {
            HMODULE hDWrite = ::LoadLibrary(L"dwrite.dll");
            if (hDWrite) {
                s_dWriteCreateFunc = (TDWriteCreateFunc)::GetProcAddress(hDWrite,
                                                                         "DWriteCreateFactory");
            }
        });
        if (!s_dWriteCreateFunc) {
            return false;
        }
    }

    if (!m_spDWriteFactory) {
        HRESULT hr = s_dWriteCreateFunc(DWRITE_FACTORY_TYPE::DWRITE_FACTORY_TYPE_SHARED,
                                        __uuidof(IDWriteFactory),
                                        (IUnknown **)&m_spDWriteFactory);
        assert(SUCCEEDED(hr));
        if (!m_spDWriteFactory || FAILED(hr)) {
            return false;
        }
    }
    return true;
}

HRESULT DWriteUtility::CreateCustomDWriteFontCollection(const std::vector<TFontResInfo> &fontInfo,
                                                        IDWriteFontCollection **ppCustomCollection)
{
    assert(m_spDWriteFactory);
    if (!m_spDWriteFactory || !ppCustomCollection) {
        return E_INVALIDARG;
    }
    auto pImpl = new TNoLockComObject<CustomCollectionLoader>;
    if (!pImpl->Initialize(m_spDWriteFactory, fontInfo)) {
        delete pImpl;
        return E_INVALIDARG;
    }
    ATL::CComPtr<IDWriteFontCollectionLoader> spCollectionLoader;
    HRESULT hr = pImpl->QueryInterface(__uuidof(IDWriteFontCollectionLoader),
                                       (void **)&spCollectionLoader);
    if (FAILED(hr)) {
        delete pImpl;
        return hr;
    }
    return CreateCustomDWriteFontCollectionImpl(spCollectionLoader, ppCustomCollection);
}

HRESULT DWriteUtility::CreateCustomDWriteFontCollection(
    std::initializer_list<TFontResInfo> fontList,
    IDWriteFontCollection **ppCustomCollection)
{
    return CreateCustomDWriteFontCollection(std::vector<TFontResInfo>(fontList),
                                            ppCustomCollection);
}

HRESULT DWriteUtility::CreateCustomDWriteFontCollection(
    const std::vector<std::wstring> &fontPath, /*字体文件路径 */
    IDWriteFontCollection **ppCustomCollection)
{
    assert(m_spDWriteFactory);
    if (!m_spDWriteFactory || !ppCustomCollection) {
        return E_INVALIDARG;
    }
    auto pImpl = new TNoLockComObject<CustomCollectionLoader>;
    if (!pImpl->Initialize(m_spDWriteFactory, fontPath)) {
        delete pImpl;
        return E_INVALIDARG;
    }
    ATL::CComPtr<IDWriteFontCollectionLoader> spCollectionLoader;
    HRESULT hr = pImpl->QueryInterface(__uuidof(IDWriteFontCollectionLoader),
                                       (void **)&spCollectionLoader);
    if (FAILED(hr)) {
        delete pImpl;
        return hr;
    }
    return CreateCustomDWriteFontCollectionImpl(spCollectionLoader, ppCustomCollection);
}

HRESULT DWriteUtility::CreateCustomDWriteFontCollection(
    std::initializer_list<const wchar_t *> fontList, /*字体文件路径 */
    IDWriteFontCollection **ppCustomCollection)
{
    assert(m_spDWriteFactory);
    if (!m_spDWriteFactory || !ppCustomCollection) {
        return E_INVALIDARG;
    }
    std::vector<std::wstring> fontPath{fontList.begin(), fontList.end()};
    return CreateCustomDWriteFontCollection(fontPath, ppCustomCollection);
}

HRESULT DWriteUtility::CreateCustomDWriteFontCollectionImpl(
    ATL::CComPtr<IDWriteFontCollectionLoader> spCollectionLoader,
    IDWriteFontCollection **ppCustomCollection)
{
    assert(m_spDWriteFactory);
    if (!m_spDWriteFactory || !spCollectionLoader || !ppCustomCollection) {
        return E_INVALIDARG;
    }

    HRESULT hr = m_spDWriteFactory->RegisterFontCollectionLoader(spCollectionLoader);
    if (FAILED(hr)) {
        return hr;
    }
    hr = m_spDWriteFactory->CreateCustomFontCollection(spCollectionLoader,
                                                       &hr,        //该参数不使用
                                                       sizeof(hr), //该参数不使用
                                                       ppCustomCollection);
    hr = m_spDWriteFactory->UnregisterFontCollectionLoader(spCollectionLoader);
    return hr;
}

ATL::CComPtr<IDWriteTextFormat> DWriteUtility::CreateTextFormat(
    const TDWriteTextAttributes &textAttr)
{
    assert(m_spDWriteFactory);
    if (!m_spDWriteFactory) {
        return nullptr;
    }
    ATL::CComPtr<IDWriteTextFormat> spTextFormat;
    HRESULT hr = m_spDWriteFactory->CreateTextFormat(textAttr.m_pFontName,
                                                     textAttr.m_pFontCollection,
                                                     textAttr.m_fontWeight,
                                                     textAttr.m_fontStyle,
                                                     textAttr.m_fontStretch,
                                                     textAttr.m_fontSize,
                                                     textAttr.m_pLocalName,
                                                     &spTextFormat);
    if (SUCCEEDED(hr)) {
        spTextFormat->SetTextAlignment(textAttr.m_textAlignment);
        spTextFormat->SetParagraphAlignment(textAttr.m_textParagraphAlignment);
        spTextFormat->SetWordWrapping(textAttr.m_textWrapping);
        spTextFormat->SetTrimming(&textAttr.m_textTrimming, textAttr.m_pTrimmingInlineObject);
        return spTextFormat;
    }
    return nullptr;
}

uint8_t *DWriteUtility::DrawToBitmap(IDWriteTextLayout *pLayout,
                                     SIZE szBitmap,
                                     const DWRITE_MATRIX &transformMx,
                                     POINT pt,
                                     COLOR32 textColor)
{
    if (!pLayout || (szBitmap.cx == 0) || (szBitmap.cy == 0)) {
        return nullptr;
    }
    HRESULT hr = E_FAIL;
    if (!m_pDWriteCustomRender) {
        m_pDWriteCustomRender = new TNoLockComObject<DWriteUtility::DWriteCustomRender>;
        m_pDWriteCustomRender->m_spDWriteFactory = m_spDWriteFactory;
    }
    m_pDWriteCustomRender->Resize(szBitmap);
    m_pDWriteCustomRender->m_textColor = textColor;
    m_pDWriteCustomRender->m_transformMx = transformMx;

    hr = pLayout->Draw(nullptr, m_pDWriteCustomRender, (FLOAT)pt.x, (FLOAT)pt.y);
    if (FAILED(hr)) {
        return nullptr;
    }
    return m_pDWriteCustomRender->m_spBitmap.get();
}

ATL::CComPtr<IDWriteFont> DWriteUtility::GetFontFromTextFormat(
    ATL::CComPtr<IDWriteTextFormat> spTextFormat)
{
    if (!spTextFormat) {
        return nullptr;
    }
    ATL::CComPtr<IDWriteFontCollection> spFontCollection;
    HRESULT hr = spTextFormat->GetFontCollection(&spFontCollection);
    if (FAILED(hr)) {
        return nullptr;
    }
    uint32_t nLength = spTextFormat->GetFontFamilyNameLength(); //不包含L'\0'
    ++nLength;
    std::unique_ptr<wchar_t[]> spFontName(new wchar_t[nLength]{});
    hr = spTextFormat->GetFontFamilyName(spFontName.get(), nLength);
    if (FAILED(hr)) {
        return nullptr;
    }

    ATL::CComPtr<IDWriteFontFamily> spFontFamily;
    uint32_t nIndex = 0;
    BOOL bFind = FALSE;
    hr = spFontCollection->FindFamilyName(spFontName.get(), &nIndex, &bFind);
    if (!bFind || FAILED(hr)) {
        return nullptr;
    }
    hr = spFontCollection->GetFontFamily(nIndex, &spFontFamily);
    if (FAILED(hr)) {
        return nullptr;
    }

    ATL::CComPtr<IDWriteFont> spFont;
    hr = spFontFamily->GetFirstMatchingFont(spTextFormat->GetFontWeight(),
                                            spTextFormat->GetFontStretch(),
                                            spTextFormat->GetFontStyle(),
                                            &spFont);
    if (FAILED(hr)) {
        return nullptr;
    }
    return spFont;
}

SHARELIB_END_NAMESPACE
