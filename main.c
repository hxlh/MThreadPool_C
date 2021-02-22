#include <stdio.h>
#include <stdlib.h>
#include "threadpool.h"

void backFunc(void *data)
{
    _sleep(200);
    printf("%d\n", (int)data);
}
int main()
{
    ThreadPool* pool=pool_getPool();
    pool_run(pool,20);
    for (size_t i = 1000; i < 1100; i++)
    {
        pool_addTask(pool,backFunc,i);
        // _sleep(100);
    }
    pool_close(pool);
    _sleep(1000);
    printf("num: %d\n",pool->tCount);
    printf("destroy: %d\n", pool_destroy(pool));
    getchar();
}
