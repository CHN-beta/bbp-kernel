#pragma once
#include "common.h"

static struct bbpCommon_spinlock
{
    spinlock_t locker;
    unsigned long flag;
    void (*lock)(struct bbpCommon_spinlock*);
    void (*unlock)(struct bbpCommon_spinlock*);
    void (*delete)(struct bbpCommon_spinlock*);
};

static void bbpCommon_spinlock_lock(struct bbpCommon_spinlock* lock)
{
    unsigned long flag;
    spin_lock_irqsave(&lock -> locker, flag);
    lock -> flag = flag;
}

static void bbpCommon_spinlock_unlock(struct bbpCommon_spinlock* lock)
{
    unsigned long flag = lock -> flag;
    spin_unlock_irqrestore(&lock -> locker, flag);
}

static void bbpCommon_spinlock_delete(struct bbpCommon_spinlock* lock)
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