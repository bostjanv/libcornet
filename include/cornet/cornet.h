#ifndef CORNET_H
#define CORNET_H

typedef void (*cornet_handler_t)(int);

struct cornet_config_t {
    int                 istcp;
    char*               server;
    int                 port;
    int                 stack_size;
    cornet_handler_t    h;
};

int cornet_init();
int cornet_fini();
int cornet_signal();
int cornet_run();
int cornet_add_server(struct cornet_config_t* config);

int cornet_read(int fd, char* buf, int len);
int cornet_write(int fd, const char* buf, int len);
int cornet_close(int fd);

#endif /* CORNET_H */
