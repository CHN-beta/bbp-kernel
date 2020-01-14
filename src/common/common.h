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

static struct bbpWorker;
typedef unsigned (*bbpExecute)(struct bbpWorker*, struct sk_buff*, bool*, bool*, bool*);
// 后三个参数用来表示是否已经可写化、是否需要重新计算 ip 校验和、是否需要重新计算 tcp 校验和。
// 如果需要重新计算 tcp 校验和，一定也会重新计算 ip 校验和（即使对应的返回值为 false）。
// 只有在返回 NF_ACCEPT 的情况下会重新计算校验和，STOLEN 的让偷走的 worker 自己去算
struct bbpWorker
{
    bbpExecute execute;        // 处理一个数据包
    bbpExecute goon;           // 在这个 worker 决定放出一个之前被它截留的数据包之后，调用这个函数来继续完成后续的处理
    void (*delete)(struct bbpWorker*);      // 销毁这个 worker
    // 在这之后是私有成员
};
typedef struct bbpWorker* (*bbpWorkerCreator)(bbpExecute);                          // 传入 goon 成员

static_assert(sizeof(int) == 4, "int is not 4 bit.");
static_assert(sizeof(short) == 2, "short is not 2 bit.");
static_assert(sizeof(unsigned long) >= sizeof(void*), "ulong is too short.");

bool bbpCommon_makeWriteable(struct sk_buff* skb)
{
    unsigned len = ((unsigned char*)ip_hdr(skb)) + ntohs(ip_hdr(skb) -> tot_len) - (unsigned char*)skb -> data;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
	if(skb_ensure_writable(skb, len) || skb -> data == 0)
#else
	if(!skb_make_writable(skb, len) || skb -> data == 0)
#endif
    {
        printk("bbp-k: bbpCommon_makeWriteable: failed.\n");
        return false;
    }
    return true;
}

void bbpCommon_csumTcp(struct sk_buff* skb)
{
    struct iphdr* iph = ip_hdr(skb);
    struct tcphdr* tcph = tcp_hdr(skb);
    tcph -> check = 0;
    iph -> check = 0;
    skb -> csum = skb_checksum(skb, iph -> ihl * 4, ntohs(iph -> tot_len) - iph -> ihl * 4, 0);
    iph -> check = ip_fast_csum((unsigned char*)iph, iph -> ihl);
    tcph -> check = csum_tcpudp_magic(iph -> saddr, iph -> daddr, ntohs(iph -> tot_len) - iph -> ihl * 4, IPPROTO_TCP, skb -> csum);
}

void bbpCommon_csumIp(struct sk_buff* skb)
{
    struct iphdr* iph = ip_hdr(skb);
    iph -> check = 0;
	iph -> check = ip_fast_csum(iph, iph -> ihl);
}

static struct bbpCommon_spinlock
{
    spinlock_t locker;
    unsigned long flag;
    void (*lock)(struct bbpCommon_spinlock*);
    void (*unlock)(struct bbpCommon_spinlock*);
    void (*delete)(struct bbpCommon_spinlock*);
};

void bbpCommon_spinlock_lock(struct bbpCommon_spinlock* lock)
{
    unsigned long flag;
    spin_lock_irqsave(&lock -> locker, flag);
    lock -> flag = flag;
}

void bbpCommon_spinlock_unlock(struct bbpCommon_spinlock* lock)
{
    unsigned long flag = lock -> flag;
    spin_unlock_irqrestore(&lock -> locker, flag);
}

void bbpCommon_spinlock_delete(struct bbpCommon_spinlock* lock)
{
    bbpCommon_free(lock);
}

static struct bbpCommon_spinlock* bbpCommon_spinlock_create(void)
{
    struct bbpCommon_spinlock* lock;
    lock = (struct bbpCommon_spinlock*)bbpCommon_malloc(sizeof(struct bbpCommon_spinlock));
    if(lock == 0)
        return 0;
    spin_lock_init(&lock -> locker);
    lock -> lock = bbpCommon_spinlock_lock;
    lock -> unlock = bbpCommon_spinlock_unlock;
    lock -> delete = bbpCommon_spinlock_delete;
    return lock;
}

#include "setting.h"
