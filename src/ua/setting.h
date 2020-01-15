#pragma once
#include "common.h"

static bool bbpUa_setting_auto = true;
module_param_named("ua_auto", bbpUa_setting_auto, bool, 0);
static char* bbpUa_setting_preserve[128];
static unsigned bbpUa_setting_preserve_n = 0;
module_param_array_named("ua_preserve", bbpUa_setting_preserve, charp, &bbpUa_setting_preserve_n, 0);
static unsigned bbpUa_setting_markCapture = 0x100;
module_param_named("ua_markCapture", bbpUa_setting_markCapture, uint, 0);
static unsigned bbpUa_setting_markAck = 0x200;
module_param_named("ua_markAck", bbpUa_setting_markAck, uint, 0);
static unsigned bbpUa_setting_alive = 1200;
module_param_named("ua_alive", bbpUa_setting_alive, uint, 0);
static unsigned bbpUa_setting_len = 2;
module_param_named("ua_len", bbpUa_setting_len, uint, 0);
static bool bbpUa_setting_debug = false;
module_param_named("ua_debug", bbpUa_setting_debug;, bool, 0);

static bool bbpUa_setting_capture(const struct sk_buff*);
static bool bbpUa_setting_ack(const struct sk_buff*);

bool bbpUa_setting_capture(const struct sk_buff* skb)
{
    if(bbpUa_setting_auto)
        return !bbpCommon_setting_local(skb) && bbpCommon_setting_send(skb)
                && ip_hdr(skb) -> protocol == IPPROTO_TCP && (tcp_hdr(skb) -> dest) == 80;
    else
        return (skb -> mark & bbpUa_setting_markCapture) == bbpUa_setting_markCapture;
}
bool bbpSetting_ack(const struct sk_buff* skb)
{
    if(bbpUa_setting_auto)
        return !bbpCommon_setting_local(skb) && bbpCommon_setting_recieve(skb)
                && ip_hdr(skb) -> protocol == IPPROTO_TCP && (tcp_hdr(skb) -> source) == 80
                && tcp_hdr(skb) -> ack;
    else
        return (skb -> mark & bbpUa_setting_markAck) == bbpUa_setting_markAck;
}