#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#define PORT 3820
#define BUFSIZE 1024

int sockfd;  // Global socket descriptor
char buffer[BUFSIZE];
int command_in_progress = 0;  // Global flag to indicate if a command is being executed
pthread_mutex_t cmd_mutex = PTHREAD_MUTEX_INITIALIZER;

// Signal handler for SIGINT (Ctrl-C)
void handle_sigint(int sig) {
    pthread_mutex_lock(&cmd_mutex);
    if (command_in_progress) {
        // Send control message to the server to stop the current command
        char ctl_msg[] = "CTL c\n";
        send(sockfd, ctl_msg, strlen(ctl_msg), 0);
    }
    // Ignore the signal if there's no command in progress (like Bash does)
    pthread_mutex_unlock(&cmd_mutex);
}

// Signal handler for SIGTSTP (Ctrl-Z)
void handle_sigtstp(int sig) {
    pthread_mutex_lock(&cmd_mutex);
    if (command_in_progress) {
        // Send control message to the server to suspend the current command
        char ctl_msg[] = "CTL z\n";
        send(sockfd, ctl_msg, strlen(ctl_msg), 0);
    }
    // Ignore the signal if there's no command in progress (like Bash does)
    pthread_mutex_unlock(&cmd_mutex);
}

void clean_buffer(char *buffer) {
    memset(buffer, 0, BUFSIZE);
}

int is_empty_input(const char *input) {
    // Check if the input string contains only spaces, tabs, or newlines
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] != ' ' && input[i] != '\t' && input[i] != '\n') {
            return 0;  // Not empty, return false
        }
    }
    return 1;  // Input is empty or whitespace only
}

void *input_handler(void *arg) {
    int rc;
    char input[BUFSIZE];

    printf("# ");  // Display prompt
    // Infinite loop to handle client commands
    while (1) {
        // Free memory
        memset(input, 0, BUFSIZE);
        // Assign fgets return value to variable ri and check
        char *ri = fgets(input, sizeof(input), stdin);
        if (ri == NULL) {
            if (errno == EINTR) {
                continue;  // Handle signals that interrupt fgets
            } else {
                break;  // Exit if EOF (Ctrl-D)
            }
        }

        // Remove trailing newline
        input[strcspn(input, "\n")] = 0;

        // Check if the input is empty (Enter key, or only spaces/tabs)
        if (is_empty_input(input)) {
            printf("# ");  // If input is empty or whitespace, print the prompt again
            continue;      // Skip sending to the server
        }

        // Handle "quit" to exit the client
        if (strcmp(input, "quit") == 0) {
            break;
        }

        pthread_mutex_lock(&cmd_mutex);
        command_in_progress = 1;  // Set flag to indicate command is running
        pthread_mutex_unlock(&cmd_mutex);

        // Send the command to the server
        snprintf(buffer, sizeof(buffer), "CMD %s\n", input);
        send(sockfd, buffer, strlen(buffer), 0);

        // Receive the response from the server
        clean_buffer(buffer);  // Zero-out buffer
        rc = recv(sockfd, buffer, BUFSIZE, 0);
        if (rc > 0) {
            // Ensure the prompt is null-terminated
            buffer[rc] = '\0';
            // Display the server's response
            printf("%s", buffer);
        } else if (rc == 0) {
            printf("Server disconnected.\n");
            break;
        }

        pthread_mutex_lock(&cmd_mutex);
        command_in_progress = 0;  // Command finished executing
        pthread_mutex_unlock(&cmd_mutex);

        // Print prompt after each command execution
        printf("# ");
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;
    pthread_t input_thread;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <IP_Address_of_Server>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Set up signal handlers for Ctrl-C and Ctrl-Z
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("Invalid IP address");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Create the input handling thread
    if (pthread_create(&input_thread, NULL, input_handler, NULL) != 0) {
        perror("Failed to create input thread");
        exit(EXIT_FAILURE);
    }

    // Wait for the input thread to finish
    pthread_join(input_thread, NULL);

    // Close the socket
    close(sockfd);
    return 0;
}
