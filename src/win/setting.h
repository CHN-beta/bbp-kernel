#pragma once
#include "../common/common.h"

static bool bbpWin_setting_auto = true;
static unsigned bbpWin_setting_markCapture = 0x100;
static unsigned bbpWin_setting_markSyn = 0x200;
module_param_named("win_auto", bbpWin_setting_auto, bool, 0);
module_param_named("win_markCapture", bbpWin_setting_markCapture, uint, 0);
module_param_named("win_markSyn", bbpWin_setting_markSyn, uint, 0);

static unsigned short bbpWin_setting_win = 128;
static unsigned char bbpWin_setting_winOffset = 7;
module_param_named("win_win", bbpWin_setting_win, ushort, 0);
module_param_named("win_winOffset", bbpWin_setting_winOffset, byte, 0);

static bool bbpWin_setting_capture(struct sk_buff* skb)
{
    if(bbpWin_setting_auto)
        return !bbpCommon_setting_local(skb) && bbpCommon_setting_send(skb);
    else
        return skb -> mark & bbpWin_setting_markCapture == bbpWin_setting_markCapture;
}

static bool bbpWin_setting_ack(struct sk_buff* skb)
{
    if(bbpWin_setting_auto)
        return !bbpCommon_setting_local(skb) && bbpCommon_setting_send(skb) && ip_hdr(skb) -> protocol == IPPROTO_TCP && tcp_hdr(skb) -> syn;
    else
        return skb -> mark & bbpWin_setting_markSyn == bbpWin_setting_markSyn;
}