#include "yashd_logger.h"

// Mutex for thread-safe logging
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Logs a command in syslog-like format to /tmp/yashd.log.
 * 
 * The format is:
 * <date and time> yashd[<Client IP>:<Port>]: <Command executed>
 * 
 * @param client_addr Pointer to the client address structure
 * @param command The command string executed by the client
 */
void log_command(struct sockaddr_in *client_addr, char *command) {
    FILE *log_file;
    char time_buffer[80];
    time_t rawtime;
    struct tm *timeinfo;
    char client_ip[INET_ADDRSTRLEN];
    int client_port;

    // Get current date and time
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_buffer, sizeof(time_buffer), "%b %d %H:%M:%S", timeinfo);

    // Get client IP and port
    inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
    client_port = ntohs(client_addr->sin_port);

    // Lock mutex before writing to the log file (thread-safe logging)
    pthread_mutex_lock(&log_mutex);

    // Open the log file in append mode
    log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        perror("Error opening log file");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    // Write the log entry
    fprintf(log_file, "%s yashd[%s:%d]: %s\n", time_buffer, client_ip, client_port, command);

    // Close the log file
    fclose(log_file);

    // Unlock the mutex after writing
    pthread_mutex_unlock(&log_mutex);
}
