#pragma once
#include "MacroDefBase.h"
#include <cstdint>
#include <cstring>
#include <windows.h>
#include <atlbase.h>
#include "WTL/atlapp.h"
#include "WTL/atlgdi.h"

SHARELIB_BEGIN_NAMESPACE

// 注意:windows的位图从低位到到高位是 bgra, RGB宏生成的是 rgba, png图片内部是 rgba
#ifndef RGBA
#define RGBA(r,g,b,a)       (COLORREF)(RGB(r,g,b) | ((DWORD)(BYTE)(a)<<24))
#endif
#ifndef GetAValue
#define GetAValue(rgba)     (LOBYTE((rgba)>>24))
#endif

class GuardDcState
{
    SHARELIB_DISABLE_COPY_CLASS(GuardDcState);
public:
    explicit GuardDcState(HDC hDc): m_hDC(hDc), m_state(::SaveDC(hDc)){}
	~GuardDcState(){ ::RestoreDC(m_hDC, m_state); }

private:
    int m_state;
    HDC m_hDC;
};

//--------------------------------------------------
class AlphaMemDc:
    public WTL::CDC
{
    SHARELIB_DISABLE_COPY_CLASS(AlphaMemDc);
public:
    AlphaMemDc();
    ~AlphaMemDc();

    /** 创建内存DC
    @param[in] hDc 外部DC
    @param[in] sz DC的大小
    @return 创建是否成功
    */
    bool Create(HDC hDc, SIZE sz);

    /** 重设DC的大小
    @param[in] sz 重设大小
    @return 操作是否成功
    */
    bool Resize(SIZE sz);

    /** 获取DC的大小
    */
    SIZE GetDCSize();

    /** 获取位图数据
    */
    uint8_t * GetBitmapData();

    /** 内存DC内容清空(全0)
    */
    void ClearZero();

    /** 销毁DC
    */
    void Destroy();

    /** 对某一个区域中的像素点进行遍历,耗费的时间与pRect的区域大小线性相关,800*600的区域,时间代价不足 500 us
    @param[in] pixelFunc: 像素处理函数:
                bool PixelFunc(const POINT& pt, uint8_t & a, uint8_t & r, uint8_t & g, uint8_t & b);
                返回false则不再遍历
    @param[in] pRect 子位图的坐标,(0,0,width,height).如果为空,表示全部位图.
    */
    template<class _PixelOp>
    bool TraversePixel(_PixelOp && pixelFunc, const RECT* pRect = nullptr);

    /** 把内存DC的bitmap复制到外部,外部的bitmap大小必须和GetDCSize()相等
    @param[in] pDest 外部存储区的地址
    @param[in] pRect 子图区域
    */
    bool CopyBitmapTo(void * pDest, const RECT * pRect = nullptr);
    
    /** 用AlphaBlend绘制内存DC到外部DC中
    @param[in] hDestDc 目标DC
    @param[in] pDest 目标区域,空的话使用全部内存DC的区域
    @param[in] pSrc 源区域,空的话使用全部内存DC的区域
    @param[in] alpha 整体透明度
    @param[in] bSrcAlpha 是否使用源透明度
    @return 操作是否成功
    */
    bool AlphaDraw(HDC hDestDc, const RECT* pDest = nullptr, const RECT * pSrc = nullptr, uint8_t alpha = 255, bool bSrcAlpha = true);

    /** 用BitBlt绘制内存DC到外部DC中
    @param[in] hDestDc 目标DC
    @param[in] pDest 目标区域,空的话使用全部内存DC的区域
    @param[in] pSrc 源区域,空的话使用全部内存DC的区域
    @param[in] dwRop 操作模式
    @return 操作是否成功
    */
    bool BitBltDraw(HDC hDestDc, const RECT* pDest = nullptr, const RECT * pSrc = nullptr, DWORD dwRop = SRCCOPY);

    /** 用StretchBlt绘制内存DC到外部DC中
    @param[in] hDestDc 目标DC
    @param[in] pDest 目标区域,空的话使用全部内存DC的区域
    @param[in] pSrc 源区域,空的话使用全部内存DC的区域
    @param[in] dwRop 操作模式
    @return 操作是否成功
    */
    bool StretchBltDraw(HDC hDestDc, const RECT* pDest = nullptr, const RECT * pSrc = nullptr, DWORD dwRop = SRCCOPY);

    /** 使用UpdateLayeredWindow将内存DC的内容绘制到层窗口中
    @param[in] hLayeredWnd 层窗口
    @param[in] pt 层窗口的左上角坐标,屏幕坐标
    @param[in] alpha 整体透明度
    @return 操作是否成功
    */
    bool UpdateLayeredWindowDraw(HWND hLayeredWnd, POINT pt, uint8_t alpha = 255);

    /** 创建内存位图,32位色深,颜色分量从低到高为bgra,行顺序为从上到下
    @param[in] hDc 源DC
    @param[in] nWidth 位图宽
    @param[in] nHeight 位图高
    @param[out,opt] ppBitmapData 创建的位图像素指针
    @return 位图句柄
    */
    static HBITMAP CreateAlphaBitmap(HDC hDc, int32_t nWidth, int32_t nHeight, void ** ppBitmapData = nullptr);

private:
    //禁用下列函数
    using WTL::CDC::DeleteDC;
    using WTL::CDC::CreateCompatibleDC;
    using WTL::CDC::CreateDC;
    using WTL::CDC::Attach;
    using WTL::CDC::Detach;

private:
    /** 位图数据
    */
    uint32_t * m_pBitmapData;

    /** DC大小
    */
    SIZE m_bitmapSize;

    /** 内存位图句柄
    */
    WTL::CBitmap m_bitmap;

    /** 旧的位图句柄
    */
    WTL::CBitmapHandle m_oldBitmap;

    /** 清理画刷
    */
    HBRUSH m_clearBrush;
};

//------------------------------------------------------------------------

template<class _PixelOp>
bool AlphaMemDc::TraversePixel(_PixelOp && pixelFunc, const RECT* pRect/* = nullptr*/)
{
    if (IsNull() || !m_pBitmapData)
    {
        return false;
    }

    RECT bitmapRect = { 0, 0, m_bitmapSize.cx, m_bitmapSize.cy };
    //获取合适的目标区域
    if (pRect)
    {
        ::IntersectRect(&bitmapRect, pRect, &bitmapRect);
        if (std::memcmp(pRect, &bitmapRect, sizeof(RECT)))
        {
            return false;
        }
    }
    if (::IsRectEmpty(&bitmapRect))
    {
        return true;
    }
    
    POINT bitmapPt{ bitmapRect.left, bitmapRect.top };

    uint32_t * pRow = m_pBitmapData + bitmapPt.y * m_bitmapSize.cx;
    uint32_t * pPixel = pRow + bitmapRect.left;
    for (; bitmapPt.y < bitmapRect.bottom; ++bitmapPt.y)
    {
        for (bitmapPt.x = bitmapRect.left;
            bitmapPt.x < bitmapRect.right; 
            ++bitmapPt.x)
        {
            if (!pixelFunc(bitmapPt,
                ((uint8_t*)pPixel)[3],
                ((uint8_t*)pPixel)[2],
                ((uint8_t*)pPixel)[1],
                ((uint8_t*)pPixel)[0]))
            {
                return true;
            }
            
            ++pPixel;
        }
        pRow += m_bitmapSize.cx;
        pPixel = pRow + bitmapRect.left;
    }

    return true;
}

SHARELIB_END_NAMESPACE
