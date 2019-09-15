﻿#pragma once
#include <Guiddef.h>
#include <Windows.h>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

//----托盘图标管理器----------------------------------------

/*!
 * \class NotifyIconMgr
 * \brief 托盘图标管理器
          使用 NOTIFYICON_VERSION_4 的方式
          回调消息: NIN_KEYSELECT,NIN_SELECT,WM_CONTEXTMENU,其它鼠标消息, NIN_BALLOONSHOW,
                  NIN_BALLOONHIDE, NIN_BALLOONTIMEOUT, NIN_BALLOONUSERCLICK
                  NIN_POPUPOPEN, NIN_POPUPCLOSE
 */
class NotifyIconMgr
{
public:
    NotifyIconMgr();
    ~NotifyIconMgr();
    bool Initialize();
    NotifyIconMgr(const NotifyIconMgr &other);
    NotifyIconMgr(NotifyIconMgr &&other);
    NotifyIconMgr &operator=(const NotifyIconMgr &other);
    NotifyIconMgr &operator=(NotifyIconMgr &&other);

    //----数据配置操作---------------------------

    // hNotifyWnd, nNotifyId, notifyGuid不清除，其它清除
    void ClearConfig();

    /** 
uMsg为WM_NULL表示不设置
uMsg 消息回调参数含义：
LOWORD(lParam) contains notification events, such as NIN_BALLOONSHOW, NIN_POPUPOPEN, or 
               WM_CONTEXTMENU.
HIWORD(lParam) contains the icon ID. Icon IDs are restricted to a length of 16 bits.
GET_X_LPARAM(wParam) returns the X anchor coordinate for notification events NIN_POPUPOPEN, 
                     NIN_SELECT, NIN_KEYSELECT, and all mouse messages between WM_MOUSEFIRST 
                     and WM_MOUSELAST. If any of those messages are generated by the keyboard, 
                     wParam is set to the upper-left corner of the target icon. For all other
                     messages, wParam is undefined.
GET_Y_LPARAM(wParam) returns the Y anchor coordinate for notification events and messages as 
                     defined for the X anchor.
*/
    void SetNotifyWndAndMsg(HWND hNotifyWnd, UINT uMsg);
    void GetNotifyWndAndMsg(HWND &hNotifyWnd, UINT &uMsg);

    /** 未设置GUID时，windows用hNotifyWnd和nNotifyId一起标识一个托盘图标；
        win7及以上可以设置GUID，此时忽略nNotifyId，只用GUID标识托盘图标。
    */
    void SetIcon(HICON icon, UINT nNotifyId);
    void GetIcon(HICON &icon, UINT &nNotifyId);
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
    void SetIcon(HICON icon, const GUID &notifyGuid);
    void GetIcon(HICON &icon, GUID &notifyGuid);
    bool GetIconBoundingRect(RECT &rcBounds);
#endif

    //最长128字符
    void SetTip(wchar_t *pTip);

    //各个值互斥
    enum InfoIconType
    {
        NoIcon = 0x00000000,      //NIIF_NONE
        InfoIcon = 0x00000001,    //NIIF_INFO
        WarningIcon = 0x00000002, //NIIF_WARNING
        ErrorIcon = 0x00000003,   //NIIF_ERROR
        UserIcon = 0x00000004,    //NIIF_USER
    };

    //各个值不互斥,可以位或
    enum InfoIconType2
    {
        NoSound = 0x00000010, //NIIF_NOSOUND
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
        LargeIcon = 0x00000020, //NIIF_LARGE_ICON
#endif
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
        RespectQuietTime = 0x00000080, //NIIF_RESPECT_QUIET_TIME
#endif
    };
    // 可以由 InfoIconType 和 InfoIconType2 构成
    void SetInfoIconType(DWORD iconType);
    DWORD GetInfoIconType();

    //最长64字符
    void SetInfoTitle(wchar_t *pInfoTitle);
    //最长256字符
    void SetInfo(wchar_t *pInfo);

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
    void SetBalloonIcon(HICON hBalloonIcon);
    HICON GetBalloonIcon();

    void SetRealTime(bool bRealTime);
    bool GetRealTime();

    void SetShowStandardTooltip(bool bStandard);
    bool GetShowStandardTooltip();
#endif

    void SetShow(bool bShow);
    bool GetShow();

    //----托盘操作---------------------------------------

    bool Add();
    bool Modify();
    bool Delete();
    bool SetFocus();

    //----辅助工具函数-----------------------------------
    static bool IsDllVersionVistaOrLater();

private:
    struct IconData;

    IconData *m_pImpl;
};

SHARELIB_END_NAMESPACE
