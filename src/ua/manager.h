#pragma once
#include "common.h"

struct bbpUa_manager
{
    struct bbpUa_stream* data[256];        // 按照两端口之和的低 8 位索引
    struct bbpCommon_spinlock* lock;
    struct timer_list timer;            // 定时器，用来定时清理不需要的流
};

struct bbpUa_manager* bbpUa_manager_new(void);
void bbpUa_manager_delete(struct bbpUa_manager*);

unsigned bbpUa_manager_execute(struct bbpUa_manager*, struct sk_buff*);   // 处理一个数据包。返回值为 bbpUa_stream_execute 的返回值。
unsigned __bbpUa_manager_execute(struct bbpUa_manager*, struct bbpUa_packet*);
void __bbpUa_manager_refresh(unsigned long);                       // 清理长时间不活动的流，参数实际上是 bbpm 的地址

void __bbpUa_manager_lock(struct bbpUa_manager*, unsigned long*);
void __bbpUa_manager_unlock(struct bbpUa_manager*, unsigned long);

struct bbpUa_manager* bbpUa_manager_new(void)
{
    struct bbpUa_manager* bbpm = (struct bbpUa_manager*)bbpMalloc(sizeof(struct bbpUa_manager));
    if(debug)
        printk("bbpUa_manager_new\n");
    if(bbpm == 0)
        return 0;
    memset(bbpm -> data, 0, sizeof(struct bbpUa_stream*) * 256);
    spin_lock_init(&bbpm -> lock);
    init_timer(&bbpm -> timer);
    bbpm -> timer.function = __bbpUa_manager_refresh;
    bbpm -> timer.data = (unsigned long)bbpm;
    bbpm -> timer.expires = jiffies + time_keepalive * HZ;
    add_timer(&bbpm -> timer);
    return bbpm;
}
void bbpUa_manager_delete(struct bbpUa_manager* bbpm)
{
    unsigned i;
    unsigned long flag;
    if(debug)
        printk("bbpUa_manager_delete\n");
    __bbpUa_manager_lock(bbpm, &flag);
    del_timer(&bbpm -> timer);
    for(i = 0; i < 256; i++)
    {
        struct bbpUa_stream* bbps = bbpm -> data[i];
        while(bbps != 0)
        {
            struct bbpUa_stream* bbps2 = bbps -> next;
            bbpUa_stream_delete(bbps);
            bbps = bbps2;
        }
    }
    __bbpUa_manager_unlock(bbpm, flag);
    bbpFree(bbpm);
}

unsigned bbpUa_manager_execute(struct bbpUa_manager* bbpm, struct sk_buff* skb)
{
    unsigned long flag;
    unsigned rtn;
    struct bbpUa_packet* bbpp;
    if(debug)
        printk("bbpUa_manager_execute\n");
    __bbpUa_manager_lock(bbpm, &flag);
    bbpp = bbpUa_packet_new(skb, bbpSetting_ack(skb));
    rtn = __bbpUa_manager_execute(bbpm, bbpp);
    if(debug)
    {
        if(rtn == NF_ACCEPT)
            printk("returned NF_ACCEPT.\n");
        else if(rtn == NF_DROP)
            printk("returned NF_DROP.\n");
        else if(rtn == NF_STOLEN)
            printk("returned NF_STOLEN.\n");
    }
    __bbpUa_manager_unlock(bbpm, flag);
    if(rtn == NF_ACCEPT || rtn == NF_DROP)
        bbpUa_packet_delete(bbpp);
    return rtn;
}
unsigned __bbpUa_manager_execute(struct bbpUa_manager* bbpm, struct bbpUa_packet* bbpp)
{
    struct bbpUa_stream *bbps, *bbps_new;

    // 搜索是否有符合条件的流
    for(bbps = bbpm -> data[bbpp -> sid]; bbps != 0; bbps = bbps -> next)
        if(bbpUa_stream_belongTo(bbps, bbpp))       // 找到了，执行即可
            return bbpUa_stream_execute(bbps, bbpp);

    // 如果运行到这里的话，那就是没有找到了，新建一个流再执行
    bbps_new = bbpUa_stream_new(bbpp);
    if(bbps_new == 0)
        return NF_ACCEPT;
    if(bbpm -> data[bbpp -> sid] == 0)
        bbpm -> data[bbpp -> sid] = bbps_new;
    else
    {
        bbpm -> data[bbpp -> sid] -> prev = bbps_new;
        bbps_new -> next = bbpm -> data[bbpp -> sid];
        bbpm -> data[bbpp -> sid] = bbps_new;
    }
    return bbpUa_stream_execute(bbps_new, bbpp);
}

void __bbpUa_manager_refresh(unsigned long param)
{
    unsigned i;
    unsigned long flag;
    struct bbpUa_manager* bbpm = (struct bbpUa_manager*)param;
    if(debug)
        printk("bbpUa_manager_refresh\n");
    __bbpUa_manager_lock(bbpm, &flag);
    for(i = 0; i < 256; i++)
    {
        struct bbpUa_stream* bbps = bbpm -> data[i];
        while(bbps != 0)
            if(!bbps -> active)
            {
                struct bbpUa_stream *bbps2 = bbps -> next;
                if(bbps -> prev != 0)
                    bbps -> prev -> next = bbps -> next;
                if(bbps -> next != 0)
                    bbps -> next -> prev = bbps -> prev;
                if(bbps == bbpm -> data[i])
                    bbpm -> data[i] = bbps -> next;
                bbpUa_stream_delete(bbps);
                bbps = bbps2;
            }
            else
            {
                bbps -> active = false;
                bbps = bbps -> next;
            }
    }
    bbpm -> timer.expires = jiffies + time_keepalive * HZ;
    add_timer(&bbpm -> timer);
    __bbpUa_manager_unlock(bbpm, flag);
}

void __bbpUa_manager_lock(struct bbpUa_manager* bbpm, unsigned long* flagp)
{
    spin_lock_irqsave(&bbpm -> lock, *flagp);
}
void __bbpUa_manager_unlock(struct bbpUa_manager* bbpm, unsigned long flag)
{
    spin_unlock_irqrestore(&bbpm -> lock, flag);
}