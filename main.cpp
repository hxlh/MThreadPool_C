#include <stdio.h>
#include "thread.h"
#include "string.h"

void backFunc(void *data)
{
    printf("%d\n",(int)data);
}
int main()
{

    ThreadPool *pool=(ThreadPool*)malloc(sizeof(ThreadPool));
    ThreadPool_Create(pool,100);
    int i;
    for(i=0;i<300;i++)
    {
        ThreadPushJob(pool,backFunc,(void*)i);
    }

    printf("hello world\n");
    getchar();
    ThreadPoolDestory(pool);
    free(pool);
    return 0;
}
