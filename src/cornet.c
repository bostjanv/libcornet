#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include "task/task.h"
#include "../include/cornet/cornet.h"

static void server_task(void*);
static void client_task(void*);

struct client_config_t {
    cornet_handler_t    h;
    int                 fd;
};

int cornet_init() {
    return 0;
}

int cornet_add_server(struct cornet_config_t* config) {
    return taskcreate(server_task, config, config->stack_size);
}

int cornet_fini() {
    return 0;
}

int cornet_signalall() {
    return tasksignalall();
}

int cornet_signal(int id) {
    return tasksignal(id);
}

int cornet_run() {
    void taskscheduler();
    taskscheduler();

    return 0;
}

void server_task(void* arg) {
    struct cornet_config_t* config;
    struct client_config_t  cconfig;
    int cfd, fd;
    char remote[16];
    int rport;

    config = (struct cornet_config_t*)arg;
    if (!config->name) {
        config->name = "server_task";
    }
    taskname(config->name);
    if ((fd = netannounce(config->istcp, config->server, config->port)) < 0) {
        fprintf(stderr, "cannot announce on tcp port %d: %s\n", config->port, strerror(errno));
        taskexitall(1);
    }

    cconfig.h = config->h;

    while((cfd = netaccept(fd, remote, &rport)) >= 0){
        fprintf(stderr, "connection from %s:%d\n", remote, rport);
        cconfig.fd = cfd;
        taskcreate(client_task, &cconfig, config->stack_size);
    }

    close(fd);
    printf("Exiting server task: %d\n", taskid());
}

void client_task(void* arg) {
    struct client_config_t* config;

    taskname("client_task");
    config = (struct client_config_t*)arg;
    config->h(config->fd);

    close(config->fd);
    printf("Exiting client task: %d\n", taskid());
}

inline
int cornet_read(int fd, char* buf, int len) {
    return fdread(fd, buf, len);
}

inline
int cornet_write(int fd, const char* buf, int len) {
    return fdwrite(fd, (char*)buf, len);
}

int cornet_close(int fd) {
    int rc;

#if 0
    if (shutdown(fd, SHUT_RDWR) == -1) {
        perror("shutdown");
        return -1;
    }
#endif
    if ((rc = close(fd)) == -1) {
        perror("close");
        return -1;
    }

    return rc;
}
