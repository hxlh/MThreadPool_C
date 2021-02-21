#include <stdio.h>
#include <stdlib.h>
#include "threadpool.h"

void backFunc(void *data)
{
    printf("%d\n", (int)data);
}
int main()
{
    Task *task1 = (Task *)malloc(sizeof(Task));
    task1->callback = backFunc;
    task1->data = 545;

    Task *task2 = (Task *)malloc(sizeof(Task));
    task2->callback = backFunc;
    task2->data = 858;
    Task *task3 = (Task *)malloc(sizeof(Task));
    task3->callback = backFunc;
    task3->data = 1010;

    Task *task4 = (Task *)malloc(sizeof(Task));
    task4->callback = backFunc;
    task4->data = 2000;

    List *tasks = NULL;
    List_add(tasks, task1->item);

    List_add(tasks, task2->item);
    List_add(tasks, task3->item);
    List_add(tasks, task4->item);

    List_del(tasks,task1->item);
    List_del(tasks,task2->item);
    List_del(tasks,task3->item);
    List_del(tasks,task4->item);
    
    Task *ttttt = NULL;
    List_take(tasks, Task, item, ttttt);
    ttttt->callback(ttttt->data);
}
