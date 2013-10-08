#include <stdio.h>
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

//static Rendez rendez;

static void server_task(void* arg);
static void io_task12(void* arg);
static void io_task21(void* arg);

int main() {
    taskcreate(server_task, 0, STACK_SIZE);
    taskscheduler();

    return 0;
}

#define SIZE 256
void server_task(void *arg) {
    int fd, cfd, rport, fd1, n;
    char remote[16];
    struct fdpair p;
    char buf[SIZE];

    fd1 = -1;

    if ((fd = netannounce(1, 0, 1111)) < 0) {
        fprintf(stderr, "cannot announce on tcp port %d: %s\n", 1111, strerror(errno));
        taskexitall(1);
    }

    while ((cfd = netaccept(fd, remote, &rport)) >= 0) {
        fprintf(stderr, "connection from %s:%d\n", remote, rport);

        if (fd1 == -1) {
            fd1 = cfd;
            n = snprintf(buf, SIZE, "There are no talkers available, wait for a while ...\n");
            fdwrite(fd1, buf, n);
            continue;
        }

        p.fd1 = fd1;
        p.fd2 = cfd;
        taskcreate(io_task12, &p, STACK_SIZE);
        taskcreate(io_task21, &p, STACK_SIZE);

        fd1 = -1;
    }

    close(fd);
    printf("Exiting server task: %d\n", taskid());
}

void io_task12(void* arg) {
    struct fdpair* p;
    int n;
    char buf[SIZE];

    p = (struct fdpair*)arg;

    n = snprintf(buf, SIZE, "There is a talker available, you can start talking now ...\n");
    fdwrite(p->fd1, buf, n);

    while ((n = fdread(p->fd1, buf, SIZE)) > 0) {
       if (fdwrite(p->fd2, buf, n) != n) {
            printf("XXX12\n");
       }
    }
    
    printf("Closing connection 1\n");
    close(p->fd1);
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
