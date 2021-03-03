#include "proxy_core.h"
#include "proxy_log.h"
#include <stdio.h>
#include <assert.h>
#include "csapp.h"

// buffer sizes
const static size_t MAX_HOSTNAME = 128;
const static size_t MAX_PORTNAME = 10;
const static size_t MAX_REQUEST_HEADERS = 40;
const static size_t MAX_HEADER_LINE = 256;

// errno for proxy_core implementation
int proxy_errno = 0;

/**
 * check test equals 1, if not, then set proxy_errno to errno_to_set
 * @param   test            expression to test
 * @param   errno_to_set    will set the proxy_errno to this value if test failed
 * @return  indicates whether the check successed (i.e., not set the proxy_errno)
*/
static int check(int test, int errno_to_set) {
    if(test)
        proxy_errno = errno_to_set;
    return !test;
}

void serve_connection(int connection_fd, rio_t *rio_buffer_ptr) {
    // read from connection_fd, parse the HTTP requests from it, and extract the address of the target host, 
    // then put these requests into new request to be written. Requests will be modified or added if needed. 
    // For simplicity, the requests are parsed through compare and flaging
    char target_host[MAX_HOSTNAME];
    char target_port[MAX_PORTNAME];
    char *requests[MAX_REQUEST_HEADERS];
    read_n_parse_client_request(rio_buffer_ptr, target_host, target_port, requests);
    if(proxy_errno == PROXY_BAD_REQUEST) {
        send_error_response();
        return;
    }
    assert(rio_buffer_ptr->rio_cnt == 0);    // logic error if remain characters in the buffer
    
    // connect to the host requested, and send the extracted requests to it
    int host_client_fd = open_clientfd(target_host, target_port);    // TODO: handle error conditions
    char *responses[MAX_REQUEST_HEADERS];
    send_to_host(host_client_fd, requests, responses);

    // TODO: free requests and responses

    // forward the responses to the connection
    send_to_connection(connection_fd, responses);
}

// calculate a constant string's length at compile time (minus 1 since '\0' will be mistakely counted)
#define static_strlen(s) ((sizeof(s) - 1) / sizeof(char))

// Allocate enough space then copy src to dest
static void alloc_n_copy(char **dest, const char *src) {
    *dest = (char*) Malloc((strlen(src) + 1) * sizeof(char));
    strcpy(*dest, src);
}

// Extract target host's hostname and port, modify the HTTP version section of request_line.
// If target_host or target_port is not provided, flush the corresponding string with "NONE" to indicate.
// If HTTP/1.1 provided, then replace it with HTTP/1.0.
void process_request_line(char *request_line, char *target_host, char *target_port) {
    // skip request method
    char *sp = request_line;
    sp = strchr(request_line, ' ');
    if(!check(sp == NULL, PROXY_BAD_REQUEST))
        return;

    // extract host name and port name
    ++sp;   // now sp point to second part of the request line
    if(*sp != '/') {    // host name (and probably port name) is included in request line
        char *proto_find = strstr(sp, "://");
        if(proto_find) {
            sp = proto_find + static_strlen("://");     // if there is protocal spec in request line, skip it
        }
        
        char *host_end;     // find end character for host string
        for(host_end = sp; *host_end != '\0' && *host_end != '\r' && *host_end != ' ' && *host_end != '/' && *host_end != ':'; ++host_end)
            ;
        strncpy(target_host, sp, host_end - sp);

        if(*host_end == ':') {
            char *port_start = host_end + 1, *port_end = port_start;
            for(; isdigit(*port_end); ++port_end)
                ;
            check(port_end == port_start, PROXY_BAD_REQUEST);
            strncpy(target_port, port_start, port_end - port_start);
        }
        else {
            strcpy(target_port, "NONE");
        }
        sp = host_end;
    }
    else {  // nothing useful in the second part, skip it
        sp = strchr(sp, ' ');
        strcpy(target_host, "NONE");
        strcpy(target_port, "NONE");
    }

    // modify http version
    char *version_num = strstr(sp, " HTTP/1.");
    if(!check(version_num == NULL, PROXY_BAD_REQUEST))
        return;
    if(*(version_num + static_strlen(" HTTP/1.")) != '0')
        *(version_num + static_strlen(" HTTP/1.")) = '0';  // alter the HTTP/1.1 to HTTP/1.0
}

// default user agent header
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void read_n_parse_client_request(rio_t *rio_buffer_ptr, char *target_host, char *target_port, char **requests) {
    proxy_errno = PROXY_FINE;
    char header_line_buffer[MAX_HEADER_LINE];
    ssize_t line_size;

    // flags
    int has_host = 0, has_user_agent = 0, has_connection = 0, has_proxy_connection = 0;
    size_t ind = 0;     // index for requests
    while((line_size = rio_readlineb(rio_buffer_ptr, header_line_buffer, MAX_HEADER_LINE)) != 0) {
        // TODO: handle rio_readlineb errors, including ECONNRESET

        if(strcmp(header_line_buffer, "\r\n") == 0)
            break;

        if(ind == 0) {  // extract host and port from request line
            process_request_line(header_line_buffer, target_host, target_port);
            if(proxy_errno == PROXY_BAD_REQUEST)
                return;
        }
        else if(strncmp(header_line_buffer, "Host", static_strlen("Host")) == 0) {
            has_host = 1;
            char *sp = strchr(header_line_buffer, ' ');
            if(!check(sp == NULL, PROXY_BAD_REQUEST))
                return;

            ++sp;
            char *host_end = sp;
            while(*host_end != '\0' && *host_end != '\r' && *host_end != ' ' && *host_end != '/' && *host_end != ':')
                ++host_end;
            if(strcmp(target_host, "NONE") == 0) {
                strncpy(target_host, sp, host_end - sp);
                target_host[host_end - sp] = '\0';
            }
            sp = host_end;
            if(strcmp(target_port, "NONE") == 0) {
                if(*sp == ':') {
                    ++sp;
                    char *port_end = sp;
                    while(isdigit(*port_end))
                        ++port_end;
                    if(!check(port_end == sp, PROXY_BAD_REQUEST))
                        return;
                    strncpy(target_port, sp, port_end - sp);
                    target_port[port_end - sp] = '\0';
                }
                else {
                    strcpy(target_port, "80");
                }
            }
        }
        else if(strncmp(header_line_buffer, "User-Agent", static_strlen("User-Agent")) == 0)
            has_user_agent = 1;
        else if(strncmp(header_line_buffer, "Connection", static_strlen("Connection")) == 0) {
            has_connection = 1;
            char *check_start = header_line_buffer + static_strlen("Connection: ");
            if(strcmp(check_start, "close") != 0)
                strcpy(check_start, "close\r\n");
        }
        else if(strncmp(header_line_buffer, "Proxy-Connection", static_strlen("Proxy-Connection")) == 0) {
            has_proxy_connection = 1;
            char *check_start = header_line_buffer + static_strlen("Proxy-Connection: ");
            if(strncmp(check_start, "close", static_strlen("close")) != 0)
                strcpy(check_start, "close\r\n");
        }

        alloc_n_copy(&requests[ind++], header_line_buffer);
    }

    char temp_buffer[MAX_HEADER_LINE];
    if(!has_host) {
        sprintf(temp_buffer, "Host: %s", target_host);
        if(strcmp(target_port, "80") != 0)
            sprintf(temp_buffer, "%s:%s", temp_buffer, target_port);
        sprintf(temp_buffer, "%s\r\n", temp_buffer);
        alloc_n_copy(&requests[ind++], temp_buffer);
    }
    if(!has_user_agent) {
        alloc_n_copy(&requests[ind++], user_agent_hdr);
    }
    if(!has_connection) {
        alloc_n_copy(&requests[ind++], "Connection: close\r\n");
    }
    if(!has_proxy_connection) {
        alloc_n_copy(&requests[ind++], "Proxy-Connection: close\r\n");
    }

    requests[ind] = NULL;
}

void send_to_host(int host_client_fd, char *requests[], char *responses[]) {
    // 
}

// dummy definition
void send_to_connection(int a, char *b[]) {

}

// dummy definition
void send_error_response() {

}