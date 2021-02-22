#include "threadpool.h"

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
            Worker* w;
            List_take(pool->workers,Worker,item,w);
            w->isExit=1;
            pthread_cond_signal(&(pool->cond));
        }
    }
}

//检测线程使用情况
void pt_pool_check(ThreadPool* pool){
    for(;;){
        pthread_mutex_lock(&pool->locker);
        //printf("num: %d\n",pool->tCount);
        if (pool->isClose>0)
        {
            pthread_mutex_unlock(&pool->locker);
            break;
        }
        //当有任务的线程小于最小线程数，并且创建的线程已超过最小线程，执行回收
        if (pool->runCount<pool->mintCount && pool->tCount>pool->mintCount)
        {
            pt_pool_recycle(pool);
        }
        pthread_mutex_unlock(&pool->locker);

        pool->timer_call(pool->expire);
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

void pool_run(ThreadPool *pool, int threadCount,void(*timer)(unsigned long long),unsigned long long expire)
{
    pool->timer_call=timer;
    pool->expire=expire;
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
    //添加定时器任务
    pool_addTask(pool,pt_pool_check,pool);
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
