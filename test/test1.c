#include <stdio.h>
#include <string.h>
#include "cornet/cornet.h"

#define STACK_SIZE 1024 * 100

static void handler1(int fd);
static void handler2(int fd);

void cornet_debug(int fd);

int main() {
    int id1, id2;

    struct cornet_config_t config1 = {
        1, NULL, 1111, STACK_SIZE, handler1, "server1"};
    struct cornet_config_t config2 = {
        1, NULL, 1112, STACK_SIZE, handler2, "server2"};

    cornet_init();
    id1 = cornet_add_server(&config1);
    id2 = cornet_add_server(&config2);
    cornet_run();
    cornet_fini();

    return 0;
}

void handler1(int fd) {
    char buf[256];
    int n, id;

    n = snprintf(buf, 256, "Welcome abroad (handler1)\n\n");
    if (n < 256) {
        cornet_write(fd, buf, n);
    }

    while ((n = cornet_read(fd, buf, 256)) > 0) {
        if (!strncmp(buf, "quit", 4)) {
            cornet_write(fd, "handler1: quit\n", sizeof("handler1: quit\n") - 1);
            cornet_signalall();
            break;
        } else if (!strncmp(buf, "signal", 6)) {
            sscanf(buf, "signal %d", &id);
            cornet_signal(id);
        } else if (!strncmp(buf, "debug", 5)) {
            cornet_debug(fd);
        }
        //fwrite(buf, 1, n, stdout);

        //cornet_write(fd, "handler1: ", sizeof("handler1: ") - 1);
        //cornet_write(fd, buf, n);
    }

    //cornet_close(fd);
    printf("handler1: connection closed\n");
}

void handler2(int fd) {
    char buf[256];
    int n;

    n = snprintf(buf, 256, "Welcome abroad (handler2)\n\n");
    if (n < 256) {
        cornet_write(fd, buf, n);
    }

    while ((n = cornet_read(fd, buf, 256)) > 0) {
        if (!strncmp(buf, "quit", 4)) {
            cornet_write(fd, "handler2: quit\n", sizeof("handler2: quit\n") - 1);
            break;
        }
        //fwrite(buf, 1, n, stdout);

        cornet_write(fd, "handler2: ", sizeof("handler2: ") - 1);
        cornet_write(fd, buf, n);
    }
    //cornet_close(fd);
    printf("handler2: connection closed\n");
}
