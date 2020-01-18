#pragma once
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kmod.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/netdevice.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/random.h>
#include <linux/moduleparam.h>
#include <linux/time.h>
#include <linux/mutex.h>
#include <linux/list.h>

typedef _Bool bool;
#define static_assert _Static_assert

static_assert(sizeof(int) == 4, "int is not 4 bit.");
static_assert(sizeof(short) == 2, "short is not 2 bit.");
static_assert(sizeof(unsigned long) >= sizeof(void*), "ulong is too short.");

static inline void* bbpCommon_malloc(unsigned size)
{
    void* p = kmalloc(size, GFP_NOWAIT);
    if(p == 0)
        printk("rkp-ua: malloc failed.\n");
    return p;
}
static inline void bbpCommon_free(void* p)
{
    kfree(p);
}



#include "worker.h"
#include "spinlock.h"
#include "timer.h"
#include "setting.h"
