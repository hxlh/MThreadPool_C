#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>
#include <malloc.h>
#include <stdio.h>
struct Worker{
    pthread_t thread;
    int terminate;
    struct Manager * pool;
    struct Worker* prev;
    struct Worker* next;

};

struct Job{
    void(*func)(void*);
    void * userData;
    struct Job* prev;
    struct Job* next;

};

struct Manager{
    pthread_cond_t jobs_cond;
    pthread_mutex_t jobs_mutex;
    struct Worker* workers;
    struct Job * jobs;
};

void ThreadPool_AddJob(struct Job* jobs,struct Job* newJob)
{

    if(jobs->next==NULL)
    {
        newJob->prev=jobs;
        newJob->next=NULL;
        jobs->next=newJob;
    }else
    {
        struct Job* next=jobs->next;
        jobs->next=newJob;
        newJob->prev=jobs;
        newJob->next=next;
        next->prev=newJob;
    }
}

void ThreadPool_RemoveJob(struct Job* job)
{
    if(job->next==NULL)
    {
        //最后一个任务
        struct Job* prev=job->prev;
        prev->next=NULL;
        job->prev=NULL;
        job->next=NULL;

    }else{
        struct Job* prev=job->prev;
        struct Job* next=job->next;
        prev->next=next;
        next->prev=prev;
    }

}

void ThreadPool_AddWorker(struct Worker* head,struct Worker* worker)
{
    if(head->next==NULL)
    {
        head->next=worker;
        worker->next=NULL;
        worker->prev=head;
    }else{
        struct Worker* next=head->next;
        head->next=worker;
        worker->prev=head;
        worker->next=next;
        next->prev=worker;
    }
}

void ThreadPool_RemoveWorker(struct Worker* worker)
{
    if(worker->next==NULL)
    {
        //最后一个worker
        struct Worker* head=worker->prev;
        head->next=NULL;

        worker->prev=NULL;
        worker->next=NULL;

    }else
    {
        struct Worker* prev=worker->prev;
        struct Worker* next=worker->next;
        prev->next=next;
        next->prev=prev;
        worker->next=NULL;
        worker->prev=NULL;

    }
}

typedef Manager ThreadPool;

void* ThreadCallback(void *arg)
{
    struct Worker*worker=(struct Worker*)arg;
    while(1)
    {
        pthread_mutex_lock(&worker->pool->jobs_mutex);
        while (worker->pool->jobs->next==NULL) {
            pthread_cond_wait(&worker->pool->jobs_cond,&worker->pool->jobs_mutex);
        }
        if(worker->terminate)
        {
            break;
        }
        struct Job* job=worker->pool->jobs->next;
        if(job==NULL)
        {
            continue;
        }
        ThreadPool_RemoveJob(job);
        pthread_mutex_unlock(&worker->pool->jobs_mutex);
        job->func(job->userData);
        free(job);
    }
    free(worker);
    pthread_mutex_unlock(&worker->pool->jobs_mutex);
}

void ThreadPool_Create(ThreadPool *pool,int threadNum)
{
    if(pool==NULL)return ;
    if(threadNum<1)threadNum=1;
    pool->jobs_mutex=PTHREAD_MUTEX_INITIALIZER;
    pool->jobs_cond=PTHREAD_COND_INITIALIZER;
    pool->jobs=(struct Job*)malloc(sizeof(struct Job));
    pool->jobs->next=NULL;
    pool->workers=(struct Worker*)malloc(sizeof(struct Worker));
    pool->workers->next=NULL;

    int i;
    for(i=0;i<threadNum;i++)
    {
        struct Worker* worker=(struct Worker*)malloc(sizeof(struct Worker));
        worker->terminate=0;
        worker->pool=pool;
        worker->next=NULL;
        worker->prev=NULL;
        ThreadPool_AddWorker(pool->workers,worker);
        pthread_create(&worker->thread,NULL,ThreadCallback,worker);
    }
}

void ThreadPushJob(ThreadPool *pool,void (*func)(void *),void* userdata)
{
    struct Job* job=(struct Job*)malloc(sizeof (struct Job));
    job->func=func;
    job->userData=userdata;
    job->next=NULL;
    job->prev=NULL;
    ThreadPool_AddJob(pool->jobs,job);

    pthread_mutex_lock(&pool->jobs_mutex);

    pthread_cond_signal(&pool->jobs_cond);

    pthread_mutex_unlock(&pool->jobs_mutex);
}

void ThreadPool_ClearJobs(ThreadPool* pool)
{
    struct Job* job=pool->jobs->next;
    for(;job!=NULL;)
    {
        ThreadPool_RemoveJob(job);
        free(job);
        job=pool->jobs->next;
    }
}

void ThreadPoolDestory(ThreadPool* pool)
{
    struct Worker* temp=pool->workers;
    for(;temp!=NULL;)
    {
        temp->terminate=1;
        temp=temp->next;
    }
    pthread_mutex_lock(&pool->jobs_mutex);
    pthread_cond_broadcast(&pool->jobs_cond);
    pthread_mutex_unlock(&pool->jobs_mutex);
    ThreadPool_ClearJobs(pool);
    struct Worker* worker=pool->workers->next;
    for(;worker!=NULL;)
    {
        ThreadPool_RemoveWorker(worker);
        free(worker);
        worker=pool->workers->next;
    }
}


#endif // THREAD_H
