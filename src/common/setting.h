#pragma once
#include "common.h"

static unsigned bbpCommon_setting_subnet[16], bbpCommon_setting_subnet_n = 0;
static unsigned bbpCommon_setting_subnetMask[16], bbpCommon_setting_subnetMask_n = 0;
module_param_array_named("common_subnet", bbpCommon_setting_subnet, uint, &bbpCommon_setting_subnet_n, 0);
module_param_array_named("common_subnetMask", bbpCommon_setting_subnetMask, uint, &bbpCommon_setting_subnetMask_n, 0);

static unsigned bbpCommon_setting_localnet[16], bbpCommon_setting_localnet_n = 0;
static unsigned bbpCommon_setting_localnetMask[16], bbpCommon_setting_localnetMask_n = 0;
module_param_array_named("common_localnet", bbpCommon_setting_localnet, uint, &bbpCommon_setting_localnet_n, 0);
module_param_array_named("common_localnetMask", bbpCommon_setting_localnetMask, uint, &bbpCommon_setting_localnetMask_n, 0);

static void bbpCommon_setting_init(void)
{
    if(bbpCommon_setting_subnet_n == 0)   // 没有设置，写入默认值 192.168.0.0/16
    {
        bbpCommon_setting_subnet_n = 1;
        bbpCommon_setting_subnet[0] = 0xC0A80000;
        bbpCommon_setting_subnetMask_n = 1;
        bbpCommon_setting_subnetMask[0] = 0xFFFF0000;
    }
    else
    {
        if(bbpCommon_setting_subnetMask_n > bbpCommon_setting_subnet_n)
            bbpCommon_setting_subnetMask_n = bbpCommon_setting_subnet_n;
        else if(bbpCommon_setting_subnetMask_n < bbpCommon_setting_subnet_n)
            for(; bbpCommon_setting_subnetMask_n < bbpCommon_setting_subnet_n; bbpCommon_setting_subnetMask_n++)
                bbpCommon_setting_subnetMask[bbpCommon_setting_subnetMask_n] = 0xFFFFFFFF;
    }

    if(bbpCommon_setting_localnet_n == 0)   // 写入默认值 0.0.0.0/8, 127.0.0.0/8
    {
        bbpCommon_setting_localnet_n = 2;
        bbpCommon_setting_localnet[0] = 0x00000000;
        bbpCommon_setting_localnet[1] = 0x7F000000;
        bbpCommon_setting_localnetMask_n = 2;
        bbpCommon_setting_localnetMask[0] = 0xFF000000;
        bbpCommon_setting_localnetMask[1] = 0xFF000000;
    }
    else
    {
        if(bbpCommon_setting_localnetMask_n > bbpCommon_setting_localnet_n)
            bbpCommon_setting_localnetMask_n = bbpCommon_setting_localnet_n;
        else if(bbpCommon_setting_localnetMask_n < bbpCommon_setting_localnet_n)
            for(; bbpCommon_setting_localnetMask_n < bbpCommon_setting_localnet_n; bbpCommon_setting_localnetMask_n++)
                bbpCommon_setting_localnetMask[bbpCommon_setting_localnetMask_n] = 0xFFFFFFFF;
    }
}

static bool bbpCommon_setting_local(struct sk_buff* skb)
{
    unsigned i;
    for(i = 0; i < bbpCommon_setting_localnet_n; i++)
        if(ntohl(ip_hdr(skb) -> saddr) & bbpCommon_setting_localnetMask[i] == bbpCommon_setting_localnet[i]
                || ntohl(ip_hdr(skb) -> daddr) & bbpCommon_setting_localnetMask[i] == bbpCommon_setting_localnet[i])
            return true;
    return false;
}

static bool bbpCommon_setting_send(struct sk_buff* skb)
{
    unsigned i;
    for(i = 0; i < bbpCommon_setting_subnet_n; i++)
        if(ntohl(ip_hdr(skb) -> saddr) & bbpCommon_setting_subnetMask[i] == bbpCommon_setting_subnet[i]
                && ntohl(ip_hdr(skb) -> daddr) & bbpCommon_setting_subnetMask[i] != bbpCommon_setting_subnet[i])
            return true;
    return false;
}

static bool bbpCommon_setting_recieve(struct sk_buff* skb)
{
    unsigned i;
    for(i = 0; i < bbpCommon_setting_subnet_n; i++)
        if(ntohl(ip_hdr(skb) -> saddr) & bbpCommon_setting_subnetMask[i] != bbpCommon_setting_subnet[i]
                && ntohl(ip_hdr(skb) -> daddr) & bbpCommon_setting_subnetMask[i] == bbpCommon_setting_subnet[i])
            return true;
    return false;
}