#pragma once
#include "common.h"

static struct bbpCommon_timer
{
    struct timer_list timer;
    unsigned interval;
    void (*delete)(struct bbpCommon_timer*);
    void (*__function)(unsigned long);
    void (*function)(void*);
    void* private;
};

static void bbpCommon_timer_delete(struct bbpCommon_timer* timer)
{
    del_timer(&timer -> timer);
    bbpCommon_free(timer);
}

static void __bbpCommon_timer_function(unsigned param)
{
    struct bbpCommon_timer* timer = (struct bbpCommon_timer*)param;
    timer -> function(timer -> private);
    timer -> timer.expires = jiffies + timer -> interval * HZ;
    add_timer(&timer -> timer);
}

static struct bbpCommon_timer* bbpCommon_timer_create(unsigned interval, void* (*function)(void*), void* private)
{
    struct bbpCommon_timer* timer;
    timer = (struct bbpCommon_timer*)bbpCommon_malloc(sizeof(struct bbpCommon_timer));
    if(timer == 0)
        return 0;
    timer -> interval = interval;
    timer -> delete = bbpCommon_timer_delete;
    timer -> __function = __bbpCommon_timer_function;
    timer -> function = function;
    timer -> private = private;
    timer_init(&timer -> timer);
    timer -> timer.function = timer -> __function;
    timer -> timer.data = (unsigned long)timer;
    timer -> timer.expires = jiffies + timer -> interval * HZ;
    add_timer(&timer -> timer);
    return timer;
}