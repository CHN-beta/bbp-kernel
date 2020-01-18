#include "../common/common.h"
#include "setting.h"
#include "map.h"
#include "packet.h"
#include "stream.h"
#include "manager.h"

static struct bbpWorker_ua
{
    bbpExecute execute;
    bbpExecute goon;
    void (*delete)(struct bbpWorker*);
    struct bbpUa_manager* manager;
};

static unsigned bbpUa_execute(struct bbpWorker* worker, struct sk_buff* skb, bool* status)
{
    struct bbpWorker_ua* worker_ua = worker;
    return bbpUa_manager_execute(worker_ua -> manager, skb, status);
}

void bbpUa_delete(struct bbpWorker* worker)
{
    struct bbpWorker_ua* worker_ua = worker;
    bbpUa_manager_delete(worker_ua -> manager);
    bbpCommon_free(worker);
}

struct bbpWorker* bbpUa_create(bbpExecute goon)
{
    struct bbpWorker_ua* worker;
    worker = bbpCommon_malloc(sizeof(struct bbpWorker_ua));
    if(worker == 0)
        return 0;
    worker -> manager = bbpUa_manager_create();
    worker -> execute = bbpUa_execute;
    worker -> goon = goon;
    worker -> delete = bbpUa_delete;

}