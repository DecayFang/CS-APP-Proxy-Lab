/*
 * proxy_core: Core implementation for the proxy server
 * including the main serve_connection method which
 * is called every iteration in the main loop.
 * Other methods that referred by serve_connection()
 * will also be included in this file.
 */ 
#ifndef _PROXY_CORE_H
#define _PROXY_CORE_H

#include "csapp.h"

/**
 * TODO: add Javadoc
*/
extern int proxy_errno;

/** possible value for proxy_errno **/

/**
 * Everything is ok
*/
#define PROXY_FINE 0

/**
 * Indicating the request from the client failed to meet the minimal requirement of the proxy,
 * i.e., the proxy even cannot find a place to forward this request to
*/
#define PROXY_BAD_REQUEST 1

/**
 * the main proxy server procedure, including read and parse the request from the client,
 * using the generated request to request from the target server, and read and send the acquired
 * contents to the client.
 * will return nothing, and every error either will be handled properly in place, or abort.
 * @param   connection_fd   the connection descriptor that represent the requesting client which is using the proxy
 * @param   rio_buffer_ptr  pointer to the rio buffer bound to the connection_fd, using solely for reading
*/
void serve_connection(int connection_fd, rio_t *rio_buffer_ptr);

/**
 * Using rio_buffer_ptr as input, read a line from the client, and extract targeting host name and port name, 
 * and modify the HTTP version to 1.0 if needed. If port name is not provided, then use 80 as default.
 * If request line cannot be parsed, then set proxy_errno to PROXY_BAD_REQUEST and return immediately.
 * If host name neither provided in request line or Host header, also set the proxy_errno to PROXY_BAD_REQUEST.
 * @param   rio_buffer_ptr  pointer to the rio buffer bound to the connection_fd, using solely for reading
 * @param   target_host     (output) targeting host name
 * @param   target_port     (output) targeting port name
 * @param   requests        array of HTTP Headers string pointers, the array is ended by NULL pointer, and every entry is a single line
*/
void read_n_parse_client_request(rio_t *rio_buffer_ptr, char *target_host, char *target_port, char **requests);

/**
 * TODO: add Javadoc
*/
void send_to_host(int, char*[], char*[]);

/**
 * TODO: add Javadoc
*/
void send_to_connection(int, char*[]);

/**
 * TODO: add Javadoc
*/
void send_error_response();

#endif // !_PROXY_CORE_H