#include "../common/common.h"
#include "setting.h"

static struct bbpWorker_win
{
    bbpExecute execute;
    bbpExecute goon;
    void (*delete)(struct bbpWorker*);
    struct bbpCommon_spinlock* lock;
    struct bbpCommon_timer* timer;
    
}