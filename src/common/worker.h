#pragma once
#include "common.h"

static struct bbpWorker;
typedef unsigned (*bbpExecute)(struct bbpWorker*, struct sk_buff*, bool*);
// 最后是三个 bool，表示是否已经可写化、是否需要重新计算 ip 校验和、是否需要重新计算 tcp 校验和。
// 如果需要重新计算 tcp 校验和，一定也会重新计算 ip 校验和（即使对应的返回值为 false）。
// 只有在返回 NF_ACCEPT 的情况下会重新计算校验和，STOLEN 的让偷走的 worker 自己去算（即使引起需要重新计算校验和的原因不是那个 worker）
struct bbpWorker
{
    bbpExecute execute;        // 处理一个数据包
    bbpExecute goon;           // 在这个 worker 决定放出一个之前被它截留的数据包之后，调用这个函数来继续完成后续的处理
    void (*delete)(struct bbpWorker*);      // 销毁这个 worker
    // 在这之后是私有成员
};
typedef struct bbpWorker* (*bbpWorkerCreator)(bbpExecute);                          // 传入 goon 成员