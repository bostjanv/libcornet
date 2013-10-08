#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "task/task.h"

#define STACK_SIZE 32768

void taskscheduler();
int  tasksignal(int id);
int  tasksignall(); 
void taskprintall();
void fdprintfdpoll();

struct fdpair {
    int fd1;
    int fd2;
};

static Channel* chan;
//static Rendez rendez;

static void acceptor_task(void* arg);
static void matcher_task(void* arg);
static void io_task12(void* arg);
static void io_task21(void* arg);

int main() {
    taskcreate(acceptor_task, 0, STACK_SIZE);
    taskscheduler();

    return 0;
}

#define SIZE 256
void acceptor_task(void *arg) {
    int fd, cfd, rport;
    char remote[16];

    chan = chancreate(sizeof(int), 10);
    taskcreate(matcher_task, 0, STACK_SIZE);

    if ((fd = netannounce(TCP, 0, 1111)) < 0) {
        fprintf(stderr, "cannot announce on tcp port %d: %s\n", 1111, strerror(errno));
        taskexitall(1);
    }

    while ((cfd = netaccept(fd, remote, &rport)) >= 0) {
        fprintf(stderr, "connection from %s:%d\n", remote, rport);

        chansendul(chan, cfd);
    }

    chanfree(chan);

    close(fd);
    printf("Exiting server task: %d\n", taskid());
}

void matcher_task(void* arg) {
    int fd1, fd2;
    struct fdpair* p;

    // TODO(bv): we could use a special value (e.g. 0 or MAX_ULONG)
    // to break out of the loop
    for (;;) {
        fd1 = chanrecvul(chan);
        fd2 = chanrecvul(chan);

        p = malloc(sizeof(*p));
        p->fd1 = fd1;
        p->fd2 = fd2;
        taskcreate(io_task12, (void*)p, STACK_SIZE);

        p = malloc(sizeof(*p));
        p->fd1 = fd1;
        p->fd2 = fd2;
        taskcreate(io_task21, (void*)p, STACK_SIZE);
    }
}

void io_task12(void* arg) {
    struct fdpair* p;
    int n;
    char buf[SIZE];

    p = (struct fdpair*)arg;

    n = snprintf(buf, SIZE, "There is a talker available, you can start talking now ...\n");
    fdwrite(p->fd1, buf, n);

    // TODO(bv): we should extend the fdwait function to allow to select
    // between different descriptors. We need this to be able to detect
    // the activities on both channels simultaneously. The scenario goes like this:
    // we are triing to read from fd1 but in the meantime fd2 disconnet. So we want
    // to break the reading from fd1.

    while ((n = fdread(p->fd1, buf, SIZE)) > 0) {
       if (fdwrite(p->fd2, buf, n) != n) {
            printf("XXX12\n");
       }
    }
    
    printf("Closing connection 1\n");
    close(p->fd1);
    free(p);
}

void io_task21(void* arg) {
    struct fdpair* p;
    int n;
    char buf[SIZE];

    p = (struct fdpair*)arg;
    
    n = snprintf(buf, SIZE, "A talker is already waiting for you, the conversation can start emediately ...\n");
    fdwrite(p->fd2, buf, n);

    while ((n = fdread(p->fd2, buf, SIZE)) > 0) {
       if (fdwrite(p->fd1, buf, n) != n) {
            printf("XXX21\n");
       }
    }
       
    printf("Closing connection 2\n");
    close(p->fd2);
    free(p);
}

#if 0

char buf[SIZE];
    int n, id;

    n = snprintf(buf, SIZE, "Welcome abroad (handler1)\n");
    cornet_write(fd, buf, n < SIZE ? n : SIZE);

    n = snprintf(buf, SIZE, "Available commands are: 'debug'', 'signal N (where N equals task's id) and 'quit'\n\n");
    cornet_write(fd, buf, n < SIZE ? n : SIZE);

    while ((n = cornet_read(fd, buf, SIZE - 1)) > 0) {
        buf[n] = 0;
        if (!strncmp(buf, "quit", 4)) {
            cornet_write(fd, "handler1: quit\n", sizeof("handler1: quit\n") - 1);
            cornet_signalall();
            break;
        } else if (!strncmp(buf, "signal", 6)) {
            id = 0;
            sscanf(buf, "signal %d", &id);
            cornet_signal(id);
        } else if (!strncmp(buf, "debug", 5)) {
            cornet_debug(fd);
        } else if (n > 2) {
            cornet_write(fd, "unknown command: ", sizeof("unknown command: ") - 1);
            cornet_write(fd, buf, n);
        }
    }
    printf("Bye (handler1)\n");
}
#endif
