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

ThreadPool *pool_getPool()
{
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    pthread_cond_init(&(pool->cond), NULL);
    pool->isClose = 0;
    pthread_mutex_init(&(pool->locker), NULL);
    pool->runCount = 0;
    pool->taskCount = 0;
    pool->tasks = NULL;
    pool->tCount = 0;
    pool->mintCount = 0;
    pool->workers = NULL;
    return pool;
};

void pt_threadRun(void *arg)
{
    Worker *w = (Worker *)arg;
    for (;;)
    {
        pthread_mutex_lock(&(w->pool->locker));
        for (; w->pool->taskCount <= 0 && w->isExit<=0  ;)
        {
            pthread_cond_wait(&(w->pool->cond), &(w->pool->locker));
        }
        if (w->isExit > 0)
        {
            break;
        }

        w->pool->runCount++;
        
        Task *task = NULL;
        List_take(w->pool->tasks, Task, item, task);

        if (task == NULL)
        {
            w->pool->runCount--;
            continue;
        }

        w->pool->taskCount--;

        pthread_mutex_unlock(&(w->pool->locker));

        task->callback(task->data);
        free(task);

        pthread_mutex_lock(&(w->pool->locker));
        w->pool->runCount--;
        if (w->isExit > 0)
        {
            break;
        }
        pthread_mutex_unlock(&(w->pool->locker));
    }

    w->pool->tCount--;

    pthread_mutex_unlock(&w->pool->locker);
    
    free(w);
    w=NULL;

    printf("thread exit\n");
}
//1.25倍增幅
void pt_pool_expand(ThreadPool *pool)
{
    int num = ceil((double)pool->tCount * 1.25);

    for (size_t i = 0; i < num; i++)
    {
        Worker *worker = (Worker *)malloc(sizeof(Worker));

        pthread_t id;

        worker->id = id;
        worker->isExit = 0;
        worker->pool = pool;
        List_add(pool->workers, worker->item);

        pthread_create(&id, NULL, pt_threadRun, (void *)worker);

        pool->tCount++;
    }
}

void pt_pool_recycle(ThreadPool *pool)
{
    int diff = pool->tCount - pool->mintCount;
    if (diff > 0)
    {
        for (size_t i = 0; i < diff; i++)
        {
            pthread_cond_signal(&(pool->cond));
        }
    }
}

void pool_addTask(ThreadPool *pool, void (*callback)(void *), void *data)
{
    pthread_mutex_lock(&pool->locker);
    if (pool->isClose != 0)
    {
        pthread_mutex_unlock(&pool->locker);
        return;
    }
    if (pool->runCount >= pool->tCount)
    {
        //增设线程
        pt_pool_expand(pool);
    }

    Task *task = (Task *)malloc(sizeof(Task));
    task->callback = callback;
    task->data = data;

    List_add(pool->tasks, task->item);
    pool->taskCount++;
    pthread_cond_signal(&pool->cond);

    pthread_mutex_unlock(&pool->locker);
};

void pool_run(ThreadPool *pool, int threadCount)
{

    for (size_t i = 0; i < threadCount; i++)
    {
        Worker *worker = (Worker *)malloc(sizeof(Worker));

        pthread_t id;

        worker->id = id;
        worker->isExit = 0;
        worker->pool = pool;
        List_add(pool->workers, worker->item);
        pthread_create(&id, NULL, pt_threadRun, (void *)worker);
        pool->tCount++;
        pool->mintCount++;
    }
}

//先close，后destroy
void pool_close(ThreadPool *pool)
{
    pthread_mutex_lock(&(pool->locker));

    pool->isClose = 1;
    for (size_t i = 0; i < pool->tCount; i++)
    {
        Worker *temp = NULL;
        List_take(pool->workers, Worker, item, temp);
        temp->isExit = 1;
    }
    pthread_cond_broadcast(&(pool->cond));
    pthread_mutex_unlock(&(pool->locker));
    
}

//返回值 ：0为销毁失败,注意请close后延时一段时间才调用，close后线程池先完成任务才退出
int pool_destroy(ThreadPool *pool)
{
    if (pool->tCount <= 0)
    {
        pthread_cond_destroy(&pool->cond);
        pthread_mutex_destroy(&pool->locker);
        free(pool);
        pool=NULL;
        return 1;
    }
    return 0;
}

#endif