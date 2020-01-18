#pragma once
#include "common.h"

static struct bbpUa_map
// 以相对序列号记录应用层数据中需要修改的部分的位置，提供修改的函数
{
    int32_t begin, length;                  // begin 为绝对序列号
    // int32_t &seq_offset = beign;         // 需要一个差不多的数值作为偏移来计算序列号谁先谁后的问题，这个偏移取为 begin
    struct bbpUa_map *prev, *next;
};

static struct bbpUa_map* bbpUa_map_new(int32_t, int32_t);                    // 两个参数分别为起始和终止绝对序列号
static void bbpUa_map_delete(struct bbpUa_map*);

static unsigned char __bbpUa_map_map(const struct bbpUa_map*, int32_t);      // 返回某个序列号对应的映射后的值。假定参数是合法的。这里的参数是相对序列号
static void bbpUa_map_modify(struct bbpUa_map**, struct bbpUa_packet**);  // 对一列序列号连续且递增的包进行修改

static void bbpUa_map_insert_begin(struct bbpUa_map**, struct bbpUa_map*);      // 在开头位置插入一个映射
static void bbpUa_map_insert_end(struct bbpUa_map**, struct bbpUa_map*);
static void bbpUa_map_refresh(struct bbpUa_map**, int32_t);                  // 对于一列序列号递增的映射，删除已经回应的映射

struct bbpUa_map* bbpUa_map_new(int32_t seql, int32_t seqr)
{
    struct bbpUa_map* bbpm = (struct bbpUa_map*)bbpCommon_malloc(sizeof(struct bbpUa_map));
    if(bbpm == 0)
        return 0;
    bbpm -> begin = seql;
    bbpm -> length = seqr - seql;
    bbpm -> prev = bbpm -> next = 0;
    return bbpm;
}
void bbpUa_map_delete(struct bbpUa_map* bbpm)
{
    bbpCommon_free(bbpm);
}

unsigned char __bbpUa_map_map(const struct bbpUa_map* bbpm, int32_t seq)
{
    if(seq < strlen(bbpUa_str_uaBbp))
        return bbpUa_str_uaBbp[seq];
    else 
        return ' ';
}
void bbpUa_map_modify(struct bbpUa_map** bbpml, struct bbpUa_packet** bbppl)
{
    const struct bbpUa_map* bbpm;
    for(bbpm = *bbpml; bbpm != 0; bbpm = bbpm -> next)
    {
        struct bbpUa_packet* bbpp;
        unsigned char* p = 0;
        int32_t seq;

        // 尝试确定第一个需要修改的包以及需要修改的开始处
        for(bbpp = *bbppl; bbpp != 0; bbpp = bbpp -> next)
            if(bbpUa_packet_seq(bbpp, bbpm -> begin) + bbpUa_packet_appLen(bbpp) > 0)
                break;
        if(bbpp == 0)
            break;
        if(bbpUa_packet_seq(bbpp, bbpm -> begin) <= 0)
        {
            p = bbpUa_packet_appBegin(bbpp) - bbpUa_packet_seq(bbpp, bbpm -> begin);
            seq = 0;
        }
        else    // p 会在稍后被设置到包的开头
            seq = bbpUa_packet_seq(bbpp, bbpm -> begin);
        
        // 开始修改
        for(; bbpp != 0; bbpp = bbpp -> next)
        {
            if(seq != 0)
                p = bbpUa_packet_appBegin(bbpp);
            for(; p != bbpUa_packet_appEnd(bbpp) && seq < bbpm -> length; p++, seq++)
                *p = __bbpUa_map_map(bbpm, seq);
            bbpp -> status[2] = true;
            if(seq == bbpm -> length)
                break;
        }
    }
}

void bbpUa_map_insert_begin(struct bbpUa_map** bbpml, struct bbpUa_map* bbpm)
{
    bbpm -> next = *bbpml;
    *bbpml = bbpm;
    if(bbpm -> next != 0)
        bbpm -> next -> prev = bbpm;
}
void bbpUa_map_insert_end(struct bbpUa_map** bbpml, struct bbpUa_map* bbpm)
{
    if(*bbpml == 0)
        *bbpml = bbpm;
    else
    {
        struct bbpUa_map* bbpm2;
        for(bbpm2 = *bbpml; bbpm2 -> next != 0; bbpm2 = bbpm2 -> next);
        bbpm2 -> next = bbpm;
        bbpm -> prev = bbpm2;
    }
}
void bbpUa_map_refresh(struct bbpUa_map** bbpml, int32_t seq)
{
    struct bbpUa_map *bbpm1, *bbpm2, *bbpm3;
    // 找到第一个不用删除的映射
    for(bbpm1 = *bbpml; bbpm1 != 0; bbpm1 = bbpm1 -> next)
        // if(bbpm -> begin + bbpm -> length > seq)        需要避免绝对值很大的负数小于绝对值很大的正数的情况
        if((int32_t)(seq - bbpm1 -> begin) - bbpm1 -> length < 0)
            break;
    // 将这个映射之前的所有映射都删除
    for(bbpm2 = *bbpml; bbpm2 != bbpm1; bbpm2 = bbpm3)
    {
        bbpm3 = bbpm2 -> next;
        bbpUa_map_delete(bbpm2);
    }
    // 修改一些指针
    if(bbpm1 != 0)
        bbpm1 -> prev = 0;
    *bbpml = bbpm1;
}