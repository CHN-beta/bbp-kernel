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

typedef _Bool bool;
#define static_assert _Static_assert

inline void* bbpMalloc(unsigned size)
{
    void* p = kmalloc(size, GFP_NOWAIT);
    if(p == 0)
        printk("rkp-ua: malloc failed.\n");
    return p;
}
inline void bbpFree(void* p)
{
    kfree(p);
}

struct bbpWorker
{
    unsigned (*execute)(struct bbpWorker*, struct sk_buff*);        // 处理一个数据包
    void (*delete)(struct bbpWorker*);                              // 销毁这个 worker
    unsigned (*goon)(struct bbpWorker*, struct sk_buff*);           // 在这个 worker 决定放出一个之前被它截留的数据包之后，调用这个函数来继续完成后续的处理
    void* private;
};