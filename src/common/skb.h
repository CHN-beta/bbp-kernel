#pragma once
#include "common.h"

static bool bbpCommon_skb_syn(struct sk_buff* skb)
{
    return tcp_hdr(skb) -> syn;
}

static bool bbpCommon_skb_ack(struct sk_buff* skb)
{
    return tcp_hdr(skb) -> ack;
}

static unsigned bbpCommon_skb_sip(struct sk_buff* skb)
{
    return ntohl(ip_hdr(skb) -> saddr);
}
static unsigned bbpCommon_skb_dip(struct sk_buff* skb)
{
    return ntohl(ip_hdr(skb) -> daddr);
}
static unsigned short bbpCommon_skb_sport(struct sk_buff* skb)
{
    return ntohs(tcp_hdr(skb) -> source);
}
static unsigned short bbpCommon_skb_dport(struct sk_buff* skb)
{
    return ntohs(tcp_hdr(skb) -> dest);
}

static unsigned short bbpCommon_skb_sid(struct sk_buff* skb)
{
    return bbpCommon_skb_sport(skb) + bbpCommon_skb_dport(skb);
}

static void bbpCommon_skb_lid(struct sk_buff* skb, unsigned* lid, bool reverse)
{
    if(!reverse)
    {
        lid[0] = bbpCommon_skb_sip(skb);
        lid[1] = bbpCommon_skb_dip(skb);
        lid[2] = (bbpCommon_skb_sport(skb) << 16) + bbpCommon_skb_dport(skb);
    }
    else
    {
        lid[0] = bbpCommon_skb_dip(skb);
        lid[1] = bbpCommon_skb_sip(skb);
        lid[2] = (bbpCommon_skb_dport(skb) << 16) + bbpCommon_skb_sport(skb);
    }
}
static bool bbpCommon_skb_lidCompare(struct sk_buff* skb, unsigned* lid, bool reverse)
{
    if(!reverse)
        return lid[0] == bbpCommon_skb_sip(skb) && lid[1] == bbpCommon_skb_dip(skb) && lid[2] == (bbpCommon_skb_sport(skb) << 16) + bbpCommon_skb_dport(skb);
    else
        return lid[0] == bbpCommon_skb_dip(skb) && lid[1] == bbpCommon_skb_sip(skb) && lid[2] == (bbpCommon_skb_dport(skb) << 16) + bbpCommon_skb_sport(skb);
}

static bool bbpCommon_skb_makeWriteable(struct sk_buff* skb)
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

static void bbpCommon_skb_csumTcp(struct sk_buff* skb)
{
    struct iphdr* iph = ip_hdr(skb);
    struct tcphdr* tcph = tcp_hdr(skb);
    tcph -> check = 0;
    iph -> check = 0;
    skb -> csum = skb_checksum(skb, iph -> ihl * 4, ntohs(iph -> tot_len) - iph -> ihl * 4, 0);
    iph -> check = ip_fast_csum((unsigned char*)iph, iph -> ihl);
    tcph -> check = csum_tcpudp_magic(iph -> saddr, iph -> daddr, ntohs(iph -> tot_len) - iph -> ihl * 4, IPPROTO_TCP, skb -> csum);
}

static void bbpCommon_skb_csumIp(struct sk_buff* skb)
{
    struct iphdr* iph = ip_hdr(skb);
    iph -> check = 0;
	iph -> check = ip_fast_csum(iph, iph -> ihl);
}