#pragma once
#include "MacroDefBase.h"
#include "UI/GraphicLayer/Layers/GraphicLayer.h"
#include <memory>

SHARELIB_BEGIN_NAMESPACE

class GraphicBitmap :
    public GraphicLayer
{
    DECLARE_RUNTIME_DYNAMIC_CREATE(GraphicBitmap, L"bitmap")

public:
    GraphicBitmap();

    /** 设置位图数据
    @param[in] pBitmapData 位图数据, B8G8R8A8格式
    @param[in] sz 位图尺寸
    */
    void SetBitmapData(const void * pBitmapData, SIZE sz);

    /** 设置是否按图片大小排布自己
    */
    void SetLayoutByPicture(bool bLayoutByPicture);

protected:
    /*
    "lrSkin" : <文本><string><皮肤名,如果没有设置Layer的Bounds，使用图片的大小作为Bounds><L"">
    */
    virtual void OnReadingXmlAttribute(const pugi::xml_attribute & attr) override;
    virtual void OnWritingAttributeToXml(pugi::xml_node node) override;
    virtual bool OnLayerMsgLayout(GraphicLayer * pLayer, bool & bLayoutChildren) override;
    virtual void Paint(D2DRenderPack & d2dRender) override;

public:
    /** 从低到高RGBA像素格式，把RGB的像素值乘以alpha之后，以BGRA顺序存储
    @param[in,out] pData 像素数据
    @param[in] nSize 数据长度
    */
    static bool RGBA2PBGRA(void * pData, SIZE sz);

    /** 获取子图数据
    @param[in] pImgData 原始图像数据
    @param[in] sz 原始图像尺寸
    @param[out] pSubImgData 子图数据,缓存区大小必须不小于子图宽*高*4
    @param[in] subRect 子图在原图中的坐标(前闭后开的区间)
    */
    static bool GetSubBitmapData(const void * pImgData, SIZE sz, void * pSubImgData, const RECT & subRect);

protected:
    std::wstring m_skin;
    bool m_bLayoutByPictureSize;
    CSize m_szBitmap;
    std::unique_ptr<uint8_t[]> m_spBitmapData;
    D2DRenderPack::CachedResID m_bitmapCacheID;
};

SHARELIB_END_NAMESPACE
