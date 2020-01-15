#pragma once
#include "common.h"

static struct bbpUa_packet
// 存储一个个数据包的类，完全被 bbpStream 和 bbpManager 包裹
{
    struct bbpUa_packet *prev, *next;
    struct sk_buff* skb;
    u_int8_t sid;
    u_int32_t lid[3];
    bool ack;
    bool status[3];
};

static struct bbpUa_packet* bbpUa_packet_create(struct sk_buff*, bool, bool*);
static void bbpUa_packet_send(struct bbpUa_packet*, struct bbpWorker*);
static void bbpUa_packet_delete(struct bbpUa_packet*, bool*);
static void bbpUa_packet_drop(struct bbpUa_packet*);

static unsigned char* bbpUa_packet_appBegin(const struct bbpUa_packet*);
static unsigned char* bbpUa_packet_appEnd(const struct bbpUa_packet*);
static unsigned bbpUa_packet_appLen(const struct bbpUa_packet*);
static int32_t bbpUa_packet_seq(const struct bbpUa_packet*, const int32_t);
static int32_t bbpUa_packet_seqAck(const struct bbpUa_packet*, const int32_t);
static u_int32_t bbpUa_packet_sip(const struct bbpUa_packet*);
static u_int32_t bbpUa_packet_dip(const struct bbpUa_packet*);
static u_int16_t bbpUa_packet_sport(const struct bbpUa_packet*);
static u_int16_t bbpUa_packet_dport(const struct bbpUa_packet*);
static bool bbpUa_packet_psh(const struct bbpUa_packet*);
static bool bbpUa_packet_syn(const struct bbpUa_packet*);
static bool bbpUa_packet_ack(const struct bbpUa_packet*);

static void bbpUa_packet_makeOffset(const struct bbpUa_packet*, int32_t*);

static void bbpUa_packet_insert_auto(struct bbpUa_packet**, struct bbpUa_packet*, int32_t offset);      // 在指定链表中插入一个包，自动根据序列号确定插入的位置
static void bbpUa_packet_insert_begin(struct bbpUa_packet**, struct bbpUa_packet*);     // 在指定链表的头部插入一个包
static void bbpUa_packet_insert_end(struct bbpUa_packet**, struct bbpUa_packet*);       // 在指定链表尾部插入一个包
static struct bbpUa_packet* bbpUa_packet_pop_begin(struct bbpUa_packet**);              // 将指定链表头部的包取出
static struct bbpUa_packet* bbpUa_packet_pop_end(struct bbpUa_packet**);                // 将指定链表尾部的包取出
static unsigned bbpUa_packet_num(struct bbpUa_packet**);                       // 返回指定链表中包的数目

static void bbpUa_packet_sendl(struct bbpUa_packet**, struct bbpWorker*);
static void bbpUa_packet_deletel(struct bbpUa_packet**, bool**);
static void bbpUa_packet_dropl(struct bbpUa_packet**);

struct bbpUa_packet* bbpUa_packet_create(struct sk_buff* skb, bool ack, bool* status)
{
    struct bbpUa_packet* bbpp = bbpMalloc(sizeof(struct bbpUa_packet));
    if(bbpp == 0)
        return 0;
    bbpp -> prev = bbpp -> next = 0;
    bbpp -> skb = skb;
    bbpp -> ack = ack;
    bbpp -> sid = (bbpUa_packet_sport(bbpp) + bbpUa_packet_dport(bbpp)) & 0xFF;
    if(!ack)
    {
        bbpp -> lid[0] = bbpUa_packet_sip(bbpp);
        bbpp -> lid[1] = bbpUa_packet_dip(bbpp);
        bbpp -> lid[2] = (bbpUa_packet_sport(bbpp) << 16) + bbpUa_packet_dport(bbpp);
    }
    else
    {
        bbpp -> lid[0] = bbpUa_packet_dip(bbpp);
        bbpp -> lid[1] = bbpUa_packet_sip(bbpp);
        bbpp -> lid[2] = (bbpUa_packet_dport(bbpp) << 16) + bbpUa_packet_sport(bbpp);
    }
    if(!__bbpUa_packet_makeWriteable(bbpp))
    {
        bbpFree(bbpp);
        return 0;
    }
    memcpy(bbpp -> status, status, sizeof(bool) * 3);
    return bbpp;
}
void bbpUa_packet_send(struct bbpUa_packet* bbpp, struct bbpWorker* worker)
{
    unsigned rtn;
    rtn = worker -> goon(worker, bbpp -> skb, bbpp -> status);
    if(rtn == NF_ACCEPT)
    {
        if(bbpp -> status[2])
            bbpCommon_csumTcp(bbpp -> skb);
        else if(bbpp -> status[1])
            bbpCommon_csumIp(bbpp -> skb);
        bbpp -> status[1] = bbpp -> status[2] = false;
        if(dev_queue_xmit(bbpp -> skb))
        {
            printk("bbp-k: bbpUa_packet_send: Send failed. Drop it.\n");
            bbpUa_packet_drop(bbpp);
        }
        else
            bbpUa_packet_delete(bbpp, 0);
    }
    else if(rtn == NF_STOLEN)
        bbpUa_packet_delete(bbpp, 0);
    else if(rtn == NF_DROP)
        bbpUa_packet_drop(bbpp);
}
void bbpUa_packet_delete(struct bbpUa_packet* bbpp, bool* status)
{
    if(status != 0)
        memcpy(status, bbpp -> status, sizeof(bool) * 3);
    bbpCommon_free(bbpp);
}
void bbpUa_packet_drop(struct bbpUa_packet* bbpp)
{
    kfree_skb(bbpp -> skb);
    bbpCommon_free(bbpp);
}

unsigned char* bbpUa_packet_appBegin(const struct bbpUa_packet* bbpp)
{
    return ((unsigned char*)tcp_hdr(bbpp -> skb)) + tcp_hdr(bbpp -> skb) -> doff * 4;
}
unsigned char* bbpUa_packet_appEnd(const struct bbpUa_packet* bbpp)
{
    return ((unsigned char*)ip_hdr(bbpp -> skb)) + ntohs(ip_hdr(bbpp -> skb) -> tot_len);
}
unsigned bbpUa_packet_appLen(const struct bbpUa_packet* bbpp)
{
    return ntohs(ip_hdr(bbpp -> skb) -> tot_len) - ip_hdr(bbpp -> skb) -> ihl * 4 - tcp_hdr(bbpp -> skb) -> doff * 4;
}
int32_t bbpUa_packet_seq(const struct bbpUa_packet* bbpp, const int32_t offset)
{
    return (int32_t)ntohl(tcp_hdr(bbpp -> skb) -> seq) - offset;
}
int32_t bbpUa_packet_seqAck(const struct bbpUa_packet* bbpp, const int32_t offset)
{
    return (int32_t)ntohl(tcp_hdr(bbpp -> skb) -> ack_seq) - offset;
}
u_int32_t bbpUa_packet_sip(const struct bbpUa_packet* bbpp)
{
    return ntohl(ip_hdr(bbpp -> skb) -> saddr);
}
u_int32_t bbpUa_packet_dip(const struct bbpUa_packet* bbpp)
{
    return ntohl(ip_hdr(bbpp -> skb) -> daddr);
}
u_int16_t bbpUa_packet_sport(const struct bbpUa_packet* bbpp)
{
    return ntohs(tcp_hdr(bbpp -> skb) -> source);
}
u_int16_t bbpUa_packet_dport(const struct bbpUa_packet* bbpp)
{
    return ntohs(tcp_hdr(bbpp -> skb) -> dest);
}
bool bbpUa_packet_psh(const struct bbpUa_packet* bbpp)
{
    return tcp_hdr(bbpp -> skb) -> psh;
}
bool bbpUa_packet_syn(const struct bbpUa_packet* bbpp)
{
    return tcp_hdr(bbpp -> skb) -> syn;
}
bool bbpUa_packet_ack(const struct bbpUa_packet* bbpp)
{
    return tcp_hdr(bbpp -> skb) -> ack;
}

bool __bbpUa_packet_makeWriteable(struct bbpUa_packet* bbpp)
{
    return bbpp -> status[0] = bbpCommon_makeWriteable(bbpp -> skb);
}

void bbpUa_packet_makeOffset(const struct bbpUa_packet* bbpp, int32_t* offsetp)
{
    *offsetp = bbpUa_packet_seq(bbpp, 0) + bbpUa_packet_appLen(bbpp);
}

void bbpUa_packet_insert_auto(struct bbpUa_packet** buff, struct bbpUa_packet* bbpp, int32_t offset)
{
    // 如果链表是空的，那么就直接加进去
    if(*buff == 0)
        *buff = bbpp;
    // 又或者，要插入的包需要排到第一个
    else if(bbpUa_packet_seq(*buff, offset) >= bbpUa_packet_seq(bbpp, offset))
    {
        (*buff) -> prev = bbpp;
        bbpp -> next = *buff;
        *buff = bbpp;
    }
    // 接下来寻找最后一个序列号小于 bbpp 的包，插入到它的后面。
    else
    {
        struct bbpUa_packet* bbpp2 = *buff;
        while(bbpp2 -> next != 0 && bbpUa_packet_seq(bbpp2 -> next, offset) < bbpUa_packet_seq(bbpp, offset))
            bbpp2 = bbpp2 -> next;
        bbpp -> next = bbpp2 -> next;
        bbpp -> prev = bbpp2;
        if(bbpp -> next != 0)
            bbpp -> next -> prev = bbpp;
        bbpp2 -> next = bbpp;
    }
}
void bbpUa_packet_insert_begin(struct bbpUa_packet** buff, struct bbpUa_packet* bbpp)
{
    if(*buff == 0)
        *buff = bbpp;
    else
    {
        (*buff) -> prev = bbpp;
        bbpp -> next = *buff;
        *buff = bbpp;
    }
}
void bbpUa_packet_insert_end(struct bbpUa_packet** buff, struct bbpUa_packet* bbpp)
{
    if(*buff == 0)
        *buff = bbpp;
    else
    {
        struct bbpUa_packet* bbpp2 = *buff;
        while(bbpp2 -> next != 0)
            bbpp2 = bbpp2 -> next;
        bbpp2 -> next = bbpp;
        bbpp -> prev = bbpp2;
    }
}
struct bbpUa_packet* bbpUa_packet_pop_begin(struct bbpUa_packet** buff)
{
    struct bbpUa_packet* bbpp = *buff;
    if(bbpp -> next == 0)
        *buff = 0;
    else
    {
        *buff = bbpp -> next;
        bbpp -> next = 0;
        (*buff) -> prev = 0;
    }
    return bbpp;
}
struct bbpUa_packet* bbpUa_packet_pop_end(struct bbpUa_packet** buff)
{
    struct bbpUa_packet* bbpp = *buff;
    while(bbpp -> next != 0)
        bbpp = bbpp -> next;
    if(bbpp == *buff)
        *buff = 0;
    else
    {
        bbpp -> prev -> next = 0;
        bbpp -> prev = 0;
    }
    return bbpp;
}
unsigned bbpUa_packet_num(struct bbpUa_packet** buff)
{
    unsigned n = 0;
    const struct bbpUa_packet* bbpp = *buff;
    while(bbpp != 0)
    {
        bbpp = bbpp -> next;
        n++;
    }
    return n;
}

void bbpUa_packet_sendl(struct bbpUa_packet** bbppl, struct bbpWorker* worker)
{
    struct bbpUa_packet *bbpp = *bbppl, *bbpp2;
    while(bbpp != 0)
    {
        bbpp2 = bbpp -> next;
        bbpUa_packet_send(bbpp, worker);
        bbpp = bbpp2;
    }
    *bbppl = 0;
}
void bbpUa_packet_deletel(struct bbpUa_packet** bbppl, bool** status)
{
    struct bbpUa_packet *bbpp = *bbppl, *bbpp2;
    unsigned i = 0;
    while(bbpp != 0)
    {
        bbpp2 = bbpp -> next;
        if(status == 0)
            bbpUa_packet_delete(bbpp, 0);
        else
            bbpUa_packet_delete(bbpp, status[i++]);
        bbpp = bbpp2;
    }
    *bbppl = 0;
}
void bbpUa_packet_dropl(struct bbpUa_packet** bbppl)
{
    struct bbpUa_packet *bbpp = *bbppl, *bbpp2;
    while(bbpp != 0)
    {
        bbpp2 = bbpp -> next;
        bbpUa_packet_drop(bbpp);
        bbpp = bbpp2;
    }
    *bbppl = 0;
}