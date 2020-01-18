#pragma once
#include "common.h"

// 这个头文件使用宏来实现类似于 vector 的功能
// 实际上 c 中实现 vector 的功能并不难，难在需要对每个类型的 vector 写一遍代码。这里通过预处理来替换字符串，简化每次的 vector 定义。

#define bbpCommon_vector(type) \
    struct bbpCommon_vector_##type

#define bbpCommon_vector_define(type) \
    struct bbpCommon_vector_##type \
    { \
        unsigned size, length; \
        struct bbpCommon_vector_##type** data; \
    }
// 因为 c 无法对 struct 定义复制构造函数，所以直接把指针拿过来就好

bbpCommon_vector_define(void);

// bbpCommon_vector(type)* bbpCommon_vector_create(type);           构造函数
// void bbpCommon_vector_createHere(bbpCommon_vector(type)*);       已经分配好空间，只是初始化
static void* __bbpCommon_vector_init()
{
    void* data = bbpCommon_malloc(size);
    memset(data, 0, size);
    return data;
}
#define bbpCommon_vector_create(type) \
    __bbpCommon_vector_malloc(sizeof(bbpCommon_vector(type)))

// void bbpCommon_vector_insert_end(bbpCommon_vector(type)* bbpv, type* element);
#define bbpCommon_vector_insert_end(type, element) \
    { \
        if()
    }
    0
