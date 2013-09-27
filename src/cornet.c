#include <stdio.h>
#include <unistd.h>
#include "libtask/task.h"

static void task1(void*);
static void task2(void*);

void taskscheduler();

int cornet_test() {
    printf("cornet_test()\n");
    
    taskcreate(task1, NULL, 4096);
    taskcreate(task2, NULL, 4096);

    taskscheduler();

    return 0;
}

void task1(void* arg) {
    int i;

    for (i = 0; i < 10; ++i) {
        printf("task1: %d\n", i);
        usleep(1000 * 1000);
        taskyield();
    }

    //taskexit(0);
}

void task2(void* arg) {
    int i;

    for (i = 0; i < 10; ++i) {
        printf("task2: %d\n", i);
        usleep(1000 * 1000);
        taskyield();
    }

    //taskexit(0);
}
