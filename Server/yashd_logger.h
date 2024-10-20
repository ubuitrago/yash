#ifndef YASHD_LOGGER_H
#define YASHD_LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netinet/in.h>

#define LOG_FILE "/tmp/yashd.log"

// Mutex for synchronizing log file writes across threads
extern pthread_mutex_t log_mutex;

// Function to log commands in syslog-like format
void log_command(struct sockaddr_in *client_addr, char *command);

#endif