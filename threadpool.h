#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define List_data(list_item, type, mber_name) \
    (type *)((char *)list_item - offsetof(type, mber_name))

#define List_take(head, type, mber_name, out)  \
    {                                          \
        struct pt_list *tmp = (head);          \
        (head) = (head)->next;                 \
        (head)->prev = NULL;                   \
        out = List_data(tmp, type, mber_name); \
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
    pool->cond = pthread_cond_init(&(pool->cond), NULL);
    pool->isClose = 0;
    pool->locker = pthread_mutex_init(&(pool->locker), NULL);
    pool->runCount = 0;
    pool->taskCount = 0;
    pool->tasks = NULL;
    pool->tCount = 0;
    pool->workers = NULL;
    return pool;
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
    }
}

void pt_threadRun(void *arg)
{
    Worker *w = (Worker *)arg;
    for (;;)
    {
        pthread_mutex_lock(&(w->pool->locker));
        for (; w->pool->taskCount > 0;)
        {
            pthread_cond_wait(&(w->pool->cond), &(w->pool->locker));
        }
        pthread_mutex_unlock(&(w->pool->locker));

        if (w->isExit!=0)
        {
            break;
        }
        
        pthread_mutex_lock(&(w->pool->locker));

        w->pool->runCount++;
        Task *task = NULL;
        List_take(w->pool->tasks, Task, item, task);
        w->pool->taskCount--;

        pthread_mutex_unlock(&(w->pool->locker));

        task->callback(task->data);
        free(task);

        pthread_mutex_lock(&(w->pool->locker));
        w->pool->runCount--;
        pthread_mutex_unlock(&(w->pool->locker));


        if (w->isExit!=0)
        {
            break;
        }
    }
    pthread_mutex_lock(w->pool->locker);

    w->pool->tCount--;
    List_del(w->pool->workers, w->item);
    free(w);

    pthread_mutex_unlock(w->pool->locker);
}

void pool_addTask(ThreadPool *pool, void (*callback)(void *), void *data)
{
    if (pool->isClose != 0)
    {
        return;
    }
    Task *task = (Task *)malloc(sizeof(Task));
    task->callback = callback;
    task->data = data;

    pthread_mutex_lock(&pool->locker);

    List_add(pool->tasks, task->item);
    pool->taskCount++;

    pthread_mutex_unlock(&pool->locker);

    pthread_cond_signal(&pool->cond);
};

//先close，后destroy
void pool_close(ThreadPool *pool)
{
    pool->isClose = 1;
    for (size_t i = 0; i < pool->tCount; i++)
    {
        Worker *temp = NULL;
        List_take(pool->workers, Worker, item, temp);
        temp->isExit = 1;
    }
    pthread_cond_broadcast(pool->cond);
}

//返回值 ：0为销毁失败
int pool_destroy(ThreadPool* pool){
    if (pool->isClose==0)
    {
        return 0;
    }
    if (pool->tCount>0)
    {
        pthread_cond_broadcast(pool->cond);
    }
    if (pool->tCount<=0)
    {
        pthread_cond_destroy(&pool->cond);
        pthread_mutex_destroy(&pool->locker);
        free(pool);
        return 1;
    }
    return 0;
}

#endif