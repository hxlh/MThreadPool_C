#include <stdio.h>
#include <stdlib.h>
#include "threadpool.h"

void timer_call(unsigned long long num){
    _sleep(num);
}
void backFunc(void *data)
{
    _sleep(1000);
    printf("%d\n", (int)data);
}
int main()
{
    ThreadPool* pool=pool_getPool();
    pool_run(pool,20,timer_call,2000);

    for (size_t i = 1000; i < 2000; i++)
    {
        pool_addTask(pool,backFunc,i);
        _sleep(10);
    }
    getchar();
}
