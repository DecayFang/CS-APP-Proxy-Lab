#include "proxy_log.h"
#include <stdlib.h>
#include "csapp.h"

FILE *logging_file_p;

void log_init(FILE *logging_file) {
    logging_file_p = logging_file;
    logging("logging system initialized!\n");
}

void logging(const char *content) {
    // using the csapp wrapping version, since any error occurred when logging should not be dismissed
    Rio_writen(fileno(logging_file_p), (void*) content, strlen(content));
}

char logging_buffer[MAX_LOGGING_BUFFER];
