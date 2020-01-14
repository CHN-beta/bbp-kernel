#pragma once
#include "../common/common.h"

static bool bbpId_setting_auto = true;
static unsigned bbpId_setting_captureMark = 0x100;
module_param_named("id_auto", bbpId_setting_auto, bool, 0);
module_param_named("id_captureMark", bbpId_setting_captureMark, uint, 0);

static bool bbpId_setting_random = false;
module_param_named("id_random", bbpId_setting_random, bool, 0);

bool bbpId_setting_capture(struct sk_buff* skb)
{
    if(bbpId_setting_auto)
        return !bbpCommon_setting_local(skb) && bbpCommon_setting_send(skb);
    else
        return skb -> mark & bbpId_setting_captureMark == bbpId_setting_captureMark;
}