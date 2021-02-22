#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define List_data(list_item, type, mber_name) \
    (type *)((char *)(list_item)-offsetof(type, mber_name))

#define List_take(head, type, mber_name, out)     \
    if ((head) == NULL)                           \
    {                                             \
        out = NULL;                               \
    }                                             \
    else                                          \
    {                                             \
        out = List_data((head), type, mber_name); \
        (head) = (head)->next;                    \
        if (head != NULL)                         \
        {                                         \
            (head)->prev = NULL;                  \
        }                                         \
    }

#define List_add(head, item)    \
    (item).prev = NULL;         \
    (item).next = NULL;         \
    if (head == NULL)           \
    {                           \
        head = &(item);         \
    }                           \
    else                        \
    {                           \
        (item).next = (head);   \
        (head)->prev = &(item); \
        (item).prev = NULL;     \
        (head) = &(item);       \
    }

#define List_del(head, item)                             \
    if ((item).prev == NULL && (item).next != NULL)      \
    {                                                    \
        (item) = *((item).next);                         \
    }                                                    \
    else if ((item).next == NULL && (item).prev != NULL) \
    {                                                    \
        (item).prev->next = NULL;                        \
    }                                                    \
    else if ((item).next == NULL && (item).prev == NULL) \
    {                                                    \
        (head) = NULL;                                   \
    }                                                    \
    else                                                 \
    {                                                    \
        (item).prev->next = (item).next;                 \
        (item).next->prev = (item).prev;                 \
    }

typedef struct pt_list
{
    struct pt_list *prev;
    struct pt_list *next;
} List;

typedef struct pt_task
{
    void (*callback)(void *);
    void *data;
    struct pt_list item;
} Task;

typedef struct ThreadPool_t
{
    //线程池是否已关闭 , 1为关闭
    int isClose;
    //正在执行任务的线程数量
    int runCount;
    //总共创建的线程数量
    int tCount;
    //最小线程数
    int mintCount;
    //任务总数
    unsigned int taskCount;
    //线程池全局锁
    pthread_mutex_t locker;
    //线程池全局条件变量
    pthread_cond_t cond;
    //任务队列
    List *tasks;
    //worker队列
    List *workers;
    //定时器
    void (*timer_call)(unsigned long long);
    unsigned long long expire;
} ThreadPool;

typedef struct pt_worker
{

    //退出标识，1为退出
    int isExit;

    pthread_t id;

    //线程池对象
    struct ThreadPool_t *pool;

    struct pt_list item;
} Worker;


ThreadPool *pool_getPool();
void pt_threadRun(void *arg);
//1.25倍增幅
void pt_pool_expand(ThreadPool *pool);
void pt_pool_recycle(ThreadPool *pool);
//检测线程使用情况
void pt_pool_check(ThreadPool* pool);
void pool_addTask(ThreadPool *pool, void (*callback)(void *), void *data);
void pool_run(ThreadPool *pool, int threadCount,void(*timer)(unsigned long long),unsigned long long expire);
void pool_close(ThreadPool *pool);
int pool_destroy(ThreadPool *pool);


#endif