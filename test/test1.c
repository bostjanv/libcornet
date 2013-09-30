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

    // Suppres warning
    (void)id1;
    (void)id2;

    return 0;
}

#define SIZE 256
void handler1(int fd) {
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

void handler2(int fd) {
    char buf[SIZE];
    int n;

    n = snprintf(buf, SIZE, "Welcome abroad (handler2)\n\n");
    cornet_write(fd, buf, n < SIZE ? n : SIZE);

    while ((n = cornet_read(fd, buf, SIZE)) > 0) {
        buf[n] = 0;
        if (!strncmp(buf, "quit", 4)) {            
            break;
        }
        //cornet_write(fd, "handler2: ", sizeof("handler2: ") - 1);
        cornet_write(fd, buf, n);
    }    
    printf("Bye (handler2)\n");
}
