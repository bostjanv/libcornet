#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "task/task.h"

#define STACK_SIZE 32768

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
    if (fdwrite(p->fd1, buf, n) != n) {
        printf("XXX11\n");
    }

    // TODO(bv): we should extend the fdwait function to allow to select
    // between different descriptors. We need this to be able to detect
    // the activities on both channels simultaneously. The scenario goes like this:
    // we are trying to read from fd1 but in the meantime fd2 disconnects. So we want
    // to break the reading from fd1.
    // Update: it seems that this is doable without select feature. But we need to be able
    // to inform the opposite site that we have been disconnected. Therefor we extended
    // the signaling mechanism to allow signaling the tasks that are beeing blocked on
    // specific file descriptor.

    while ((n = fdread(p->fd1, buf, SIZE)) > 0) {
        if (fdwrite(p->fd2, buf, n) != n) {
            printf("XXX12\n");
        }
    }
   
    if (n == 0 && tasksignaled()) {
        // We have been signaled.
        tasksignalreset();
        chansendul(chan, p->fd1);
    } else {
        printf("Closing connection 1\n");
        n = snprintf(buf, SIZE, "1 hanged up ..., wait for another one\n");
        fdwrite(p->fd2, buf, n);
        tasksignalfd(p->fd2);
        close(p->fd1);
    }

    free(p);
}

void io_task21(void* arg) {
    struct fdpair* p;
    int n;
    char buf[SIZE];

    p = (struct fdpair*)arg;
    
    n = snprintf(buf, SIZE, "A talker is already waiting for you, the conversation can start emediately ...\n");
    if (fdwrite(p->fd2, buf, n) != n) {
        printf("XXX21\n");
    }

    while ((n = fdread(p->fd2, buf, SIZE)) > 0) {
        if (fdwrite(p->fd1, buf, n) != n) {
            printf("XXX22\n");
        }
    }
      
    if (n == 0 && tasksignaled()) {
        // We have been signaled.
        tasksignalreset();
        chansendul(chan, p->fd2);
    } else {
        printf("Closing connection 2\n");
        n = snprintf(buf, SIZE, "2 hanged up ..., wait for another one\n");
        fdwrite(p->fd1, buf, n);
        tasksignalfd(p->fd1);
        close(p->fd2);
    }
    
    free(p);
}
