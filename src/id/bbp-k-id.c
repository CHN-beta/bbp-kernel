#include "../common/common.h"
#include "setting.h"

static struct bbpWorker_id
{
    bbpExecute execute;
    bbpExecute goon;
    void (*delete)(struct bbpWorker*);
    struct bbpCommon_spinlock* lock;
    unsigned short id_next;
};

static unsigned bbpId_execute(struct bbpWorker* worker_bbp, struct sk_buff* skb, bool* writeable, bool* ipcheck, bool* tcpcheck)
{
    struct bbpWorker_id* worker = worker_bbp;
    if(!bbpId_setting_capture(skb))
        return NF_ACCEPT;
    if(!*writeable)
    {
        *writeable = bbpCommon_makeWriteable(skb);
        if(!*writeable)
            return NF_ACCEPT;
    }
    if(bbpId_setting_random)
        get_random_bytes(&ip_hdr(skb) -> id, 2);
    else
    {

        worker -> lock -> lock(worker -> lock);
        ip_hdr(skb) -> id = htons(worker -> id_next);
        *ipcheck = true;
        worker -> id_next++;
        worker -> lock -> lock(worker -> lock);
    }
    return NF_ACCEPT;
}

static void bbpId_delete(struct bbpWorker* worker_bbp)
{
    struct bbpWorker_id* worker = worker_bbp;
    worker -> lock -> delete(worker -> lock);
    bbpCommon_free(worker);
}

struct bbpWorker* bbpId_create(bbpExecute goon)
{
    struct bbpWorker_id* worker;
    worker = bbpCommon_malloc(sizeof(struct bbpWorker_id));
    if(worker == 0)
        return 0;
    worker -> execute = bbpId_execute;
    worker -> goon = goon;
    worker -> delete = bbpId_delete;
    worker -> lock = bbpCommon_spinlock_create();
    return worker;
}