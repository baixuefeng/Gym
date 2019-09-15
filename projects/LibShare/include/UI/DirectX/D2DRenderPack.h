#pragma once
#include <list>
#include <map>
#include <memory>
#include <stack>
#include <vector>
#include <atlbase.h>
#include <atlcomcli.h>
#include <d2d1helper.h>
#include "MacroDefBase.h"
#include "UI/DirectX/D2D1Misc.h"

//不推荐使用系统的, 使用D2DRenderPack中封装的
#pragma deprecated(PushAxisAlignedClip)
#pragma deprecated(PopAxisAlignedClip)
#pragma deprecated(PushLayer)
#pragma deprecated(PopLayer)
#pragma deprecated(SaveDrawingState)
#pragma deprecated(RestoreDrawingState)

SHARELIB_BEGIN_NAMESPACE

class D2DRenderPack
{
    SHARELIB_DISABLE_COPY_CLASS(D2DRenderPack);

public:
    //----辅助类--------------------------

    //PushClipRect, PopClipRect配对操作辅助
    class RectClipGuard
    {
    public:
        RectClipGuard(D2DRenderPack &render, const RECT &rcClip, bool bAntiAliasing = false);
        ~RectClipGuard();

    private:
        D2DRenderPack &m_render;
    };

    //PushD2DLayer, PopD2DLayer配对操作辅助
    class RenderLayerGuard
    {
    public:
        RenderLayerGuard(D2DRenderPack &render, const D2D1_LAYER_PARAMETERS &layerParam);
        ~RenderLayerGuard();

    private:
        D2DRenderPack &m_render;
    };

    //PushDrawingState, PopDrawingState配对操作辅助
    class DrawingStateGuard
    {
    public:
        explicit DrawingStateGuard(D2DRenderPack &render);
        ~DrawingStateGuard();

    private:
        D2DRenderPack &m_render;
    };

    //自动释放缓存资源
    class CachedResID
    {
    public:
        CachedResID();
        ~CachedResID();
        CachedResID(CachedResID &&other);
        CachedResID &operator=(CachedResID &&other);
        void ReleaseD2DResourceCache(); //释放缓存的资源,可以不调用,析构时会调用
        operator uint64_t();

        void Detach(); //不释放缓存的资源,脱离管理的缓存ID
    private:
        friend class D2DRenderPack;
        uint64_t m_cacheID;
        D2DRenderPack *m_pRenderPack;
    };

public:
    D2DRenderPack();
    ~D2DRenderPack();

    // 创建D2D工厂, 可以不调用, 第一次PrepareRender时会自动调用
    bool CreateD2DFactory();

    // 渲染类型
    enum TRenderType
    {
        Auto,     //自动选择(硬件优先)
        Hardware, //硬件
        Software  //软件
    };

    /** 准备绘图环境, 该函数成功执行之后, public成员才可以使用
    @param[in] hWnd 窗口句柄, nullptr表示离屏渲染
    @param[in] szRender Render大小
    @param[in] renderType Render类型
    @param[in] fDPI DPI倍率，如果为0表示自动适应系统的DPI设置
    @return 绘图环境是否准备成功
    */
    bool PrepareRender(HWND hWnd,
                       const SIZE &szRender,
                       TRenderType renderType = TRenderType::Auto,
                       float fDPI = 1.0f);

    /** 清理设备依赖的Render资源,当PrepareRender传入的窗口销毁时,必须清理Render资源
    */
    void ClearRenderRes();

    /** 封装D2D的 PushAxisAlignedClip 操作
    */
    void PushClipRect(const RECT &rcClip, bool bAntiAliasing = false);

    /** 封装D2D的 PopAxisAlignedClip 操作
    */
    void PopClipRect();

    /** 封装D2D的 PushLayer 操作,内部使用缓存技术
    @param[in] layerParam Layer的参数
    */
    void PushD2DLayer(const D2D1_LAYER_PARAMETERS &layerParam);

    /** 封装D2D的 PopLayer 操作,内部使用缓存技术
    */
    void PopD2DLayer();

    /** 当前的裁剪区域是否为空
    */
    bool IsClipEmpty();

    /** 测试矩形是否在裁剪区域内(交集非空)
    */
    bool IsRectInClip(const RECT &rcTest);

    /** 封装D2D的 SaveDrawingState,内部使用缓存技术
    */
    void PushDrawingState();

    /** 封装D2D的 RestoreDrawingState,内部使用缓存技术
    */
    void PopDrawingState();

    /** 创建一个新的位图，位图的像素格式为 b8g8r8a8
    @param[in] sz 需要的位图尺寸
    @param[in,opt] pBitmapData 位图数据指针(格式:b8g8r8a8),如果非空,则用它初始化位图
    @return 创建成功返回非空，否则为空
    */
    ATL::CComPtr<ID2D1Bitmap> CreateD2DBitmap(const SIZE &sz, const void *pBitmapData);

    /** 缓存D2D资源，避免每次绘制都重新创建，缓存的资源如果是设备相关的，当设备失效时会自动清除
    注意, D2DRenderPack的生命期必须大于CachedResID !
    @param[in] pResource D2D资源
    @return 返回资源唯一标识ID, 数值必大于0
    */
    CachedResID CacheD2DResource(ID2D1Resource *pResource);

    /** 获取缓存的资源，0为无效ID，当用正确ID也查询不到资源时，说明设备曾经失效被清理掉了，应该重新创建、重新缓存
    @param[in] resID 资源ID
    @return 资源
    */
    ATL::CComPtr<ID2D1Resource> GetCachedD2DResource(const CachedResID &resID);

    /** 释放D2D资源缓存
    @param[in] resID 资源ID
    */
    void ReleaseD2DResourceCache(const CachedResID &resID);

    /** 释放所有D2D资源缓存
    */
    void ReleaseAllD2DResourceCache();

public:
    /** 计算弧的起点、终点坐标。D2D中的弧的表示方式是用起点、终点、半径综合表示，
    缺少 Gdiplus 中用椭圆和旋转角度综合表示的方式，该辅助函数就是通过椭圆和旋转角度
    计算出弧的起点、终点。
    @param[in] ellipseBound 包围椭圆的矩形
    @param[in] angleStart 起始角度，表示相对于 x正向 向 y正向 旋转的角度
    @param[in] angleSweep 弧所扫过的角度，方向：x正向 向 y正向
    @param[out] ptStart 计算出的弧的起点
    @param[out] ptEnd 计算出的弧的终点
    @param[out] szRadius 椭圆半径
    */
    static void CalculateArcPoint(const RECT &ellipseBound,
                                  float angleStart,
                                  float angleSweep,
                                  D2D1_POINT_2F &ptStart,
                                  D2D1_POINT_2F &ptEnd,
                                  D2D1_SIZE_F &szRadius);

    /** 计算出画余弦[0,Pi]曲线所需的Bezire控制点.([Pi,2Pi]的部分,只需把y坐标求负再平移即可)
    @param[in] fPiLength Pi代表的长度,必须大于0
    @param[out] bezierPts Bezire点数组,固定为4个元素
    */
    static void CalculateCosBezire(float fPiLength, D2D1_POINT_2F *pBezierPts);

private:
    /** 计算矩形经坐标转换后的与坐标轴对齐的外围矩形
    @param[in,out] 要计算的矩形 
    */
    void CalculateTransRect(CD2DRectF &rect);

    /** 保存当前的裁剪矩形
    */
    void SaveCurrentClip(const CD2DRectF &rcClip);

    /** 恢复当前的裁剪矩形
    */
    void RestoreCurrentClip();

    /** 清理用户资源缓存
    */
    void ClearUserRes();

public:
    //设备无关的部分
    ATL::CComPtr<ID2D1Factory> m_spD2DFactory;

    //设备相关的部分
    ATL::CComPtr<ID2D1RenderTarget> m_spD2DRender;
    ATL::CComPtr<ID2D1SolidColorBrush> m_spSolidBrush; //公用实心画刷

private:
    //用户资源ID
    uint64_t m_nUserResID;
    //用户资源缓存,ClearRenderRes时,会自动把设备相关资源删除. 缓存起来避免反复绘制反复创建
    std::map<uint64_t, ATL::CAdapt<ATL::CComPtr<ID2D1Resource>>> m_userResCache;

    //设备无关的部分
    std::list<ATL::CAdapt<ATL::CComPtr<ID2D1DrawingStateBlock>>> m_statePool;

    //矩形裁剪区域栈
    std::stack<CD2DRectF, std::vector<CD2DRectF>> m_clipStack;

    //设备相关的部分
    class OffScreenWnd;
    std::unique_ptr<OffScreenWnd> m_spOffScreenWnd;
    ATL::CComPtr<ID2D1HwndRenderTarget> m_spD2DHwndRender;
    ATL::CComPtr<ID2D1BitmapRenderTarget> m_spD2DOffScreenRender;
    std::list<ATL::CAdapt<ATL::CComPtr<ID2D1Layer>>> m_layerPool;
};

SHARELIB_END_NAMESPACE
