/**
 * proxy_log: the logging system for the proxy server.
 * this system is forced to use Robust IO(rio) provided
 * by csapp as output.
 * IMPORTANT: if logging system is in error state, the proxy should terminate
*/
#ifndef _PROXY_LOG_H
#define _PROXY_LOG_H

#include <stdio.h>
#include "csapp.h"

#define MAX_LOGGING_BUFFER MAXLINE

/**
 * initialize the logging system, bind the system to the file to write.
 * User should not call this method more than once, or free the logging_file pointer.
 * Kill the process if any unhandlable error detected.
 * @param   logging_file   the file pointer that the logging system will hold
*/  
void log_init(FILE *logging_file);

/**
 * write the log with provided content.
 * Using RIO package internally. User may pre-format the content using sscanf. 
 * MUST be called after log_init(). 
 * Kill the process if any unhandlable error detected. 
 * @param   content   the string that will be written to the log
*/
void logging(const char *content);

/**
 * buffer provided to user to avoid overhead of repeativly allocating the buffer
*/
extern char logging_buffer[MAX_LOGGING_BUFFER];

#endif // !_PROXY_LOG_H