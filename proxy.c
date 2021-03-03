#include <stdio.h>
#include "proxy_core.h"
#include "proxy_log.h"
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

int main(int argc, char *argv[])
{
    log_init(stderr);

    if(argc != 2) {
        sprintf(logging_buffer, "usage: %s [port]", argv[0]);
        logging(logging_buffer);
        return -1;
    }

    int listen_fd = Open_listenfd(argv[2]);

    socklen_t client_socklen;
    struct sockaddr_storage client_addr;
    rio_t rio_buffer;
    while(1) {
        int connection_fd = accept(listen_fd, (struct sockaddr*) &client_addr, &client_addr);   // TODO: handle error conditions
        rio_readinitb(&rio_buffer, connection_fd);  // TODO: move this buffer into threading procedure
        serve_connection(connection_fd, &rio_buffer);
    }

    return 0;
}
