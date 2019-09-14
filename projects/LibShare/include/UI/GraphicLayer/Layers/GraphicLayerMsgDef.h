#pragma once
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

//----------------------------------------------------------------
//下面是预先占用的消息定义, 用于从真实窗口转接消息, GraphicLayer并不直接使用, 外部也不要使用

//定时器消息
#define LAYER_WM_TIMER_NOTIFY                          (0x7FFF - 1)

//内部使用消息, wParam: GraphicLayer *
#define LAYER_WM_INTERNAL_USE                          (0x7FFF - 2)

//----------------------------------------------------------------

class GraphicLayer;

/** GraphicLayer消息处理接口类, 第一个参数固定为接收消息的目标Layer,返回值固定表示是否处理过该消息了
*/
struct __declspec(novtable) LayerMsgCallback
{
    /** 用户自定义消息,UI框架不会发送此消息
    */
    virtual bool OnLayerMsgUserDefined(GraphicLayer * pLayer, void * pUserData)
    {
        (void)pLayer;
        (void)pUserData;
        return false;
    }

    /** 重新排布消息响应
    @param[in] bLayoutChildren 是否排布子Layer,默认为true
    */
    virtual bool OnLayerMsgLayout(GraphicLayer * pLayer, bool & bLayoutChildren)
    {
        (void)pLayer;
        (void)bLayoutChildren;
        return false;
    }

    /** 复用系统消息WM_MOUSEFIRST~WM_MOUSELAST,各参数含义不作任何改变,但坐标已经转换为相对pLayer自身的坐标
    */
    virtual bool OnLayerMsgMouse(GraphicLayer * pLayer, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT & lResult)
    {
        (void)pLayer;
        (void)uMsg;
        (void)wParam;
        (void)lParam;
        (void)lResult;
        return false;
    }

    /** 复用系统消息WM_KEYFIRST~WM_KEYLAST,各参数含义不作任何改变
    */
    virtual bool OnLayerMsgKey(GraphicLayer * pLayer, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT & lResult)
    {
        (void)pLayer;
        (void)uMsg;
        (void)wParam;
        (void)lParam;
        (void)lResult;
        return false;
    }

    /** 鼠标移入、移出消息, 边界触发模式
    @param[in] bIn true表示进入,false表示移出
    */
    virtual bool OnLayerMsgMouseInOut(GraphicLayer * pLayer, bool bIn)
    {
        (void)pLayer;
        (void)bIn;
        return false;
    }

    /** 获得焦点消息
    @param[in] pLayer 获得焦点的目标Layer,非nullptr
    @param[in] pLostFocus 失去焦点的Layer,可能为nullptr
    */
    virtual bool OnLayerMsgSetFocus(GraphicLayer * pLayer, GraphicLayer * pLostFocus)
    {
        (void)pLayer;
        (void)pLostFocus;
        return false;
    }

    /** 失去焦点消息
    @param[in] pLayer 失去焦点的目标Layer,非nullptr
    @param[in] pLostFocus 获得焦点的Layer,可能为nullptr
    */
    virtual bool OnLayerMsgKillFocus(GraphicLayer * pLayer, GraphicLayer * pGetFocus)
    {
        (void)pLayer;
        (void)pGetFocus;
        return false;
    }

    /** 定时器消息
    @param[in] pLayer 目标Layer
    @param[in] hTimer CreateLayerTimer返回的 HANDLE
    */
    virtual bool OnLayerMsgTimer(GraphicLayer * pLayer, HANDLE hTimer)
    {
        (void)pLayer;
        (void)hTimer;
        return false;
    }
};

SHARELIB_END_NAMESPACE
