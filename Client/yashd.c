#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#define PORT 3820
#define BUFSIZE 1024

int sockfd, newsockfd;  // Socket descriptors for server and client
pid_t child_pid;        // PID of the forked child

void handle_sigint(int sig) {
    // Close the server sockets on SIGINT (Ctrl-C)
    if (sockfd > 0) close(sockfd);
    if (newsockfd > 0) close(newsockfd);
    printf("Server terminated.\n");
    exit(0);
}

void handle_client(int client_sockfd) {
    char buffer[BUFSIZE];
    char command[BUFSIZE];
    int n;

    while (1) {
        // Clear the buffers
        memset(buffer, 0, BUFSIZE);
        memset(command, 0, BUFSIZE);

        // Read the message from the client
        n = recv(client_sockfd, buffer, BUFSIZE, 0);
        if (n <= 0) {
            printf("Client disconnected.\n");
            break;
        }

        // Handle CMD messages
        if (strncmp(buffer, "CMD ", 4) == 0) {
            strcpy(command, buffer + 4);  // Extract the command after "CMD "
            command[strcspn(command, "\n")] = 0;  // Remove newline character

            printf("Executing command: %s\n", command);

            // Fork a child to execute the command
            child_pid = fork();
            if (child_pid == 0) {
                // Redirect stdout to the client socket
                dup2(client_sockfd, STDOUT_FILENO);
                dup2(client_sockfd, STDERR_FILENO);

                // Execute the command
                execlp("/bin/sh", "sh", "-c", command, (char *)NULL);
                perror("execlp failed");
                exit(EXIT_FAILURE);
            } else {
                // Wait for the child to finish executing the command
                waitpid(child_pid, NULL, 0);

                // After command execution, send the prompt to the client
                send(client_sockfd, "\n#", 3, 0);
            }
        }

        // Handle CTL messages (CTRL-C, CTRL-Z)
        else if (strncmp(buffer, "CTL c", 5) == 0) {
            // Forward Ctrl-C (SIGINT) to the child process
            if (child_pid > 0) {
                kill(child_pid, SIGINT);
            }
        } else if (strncmp(buffer, "CTL z", 5) == 0) {
            // Forward Ctrl-Z (SIGTSTP) to the child process
            if (child_pid > 0) {
                kill(child_pid, SIGTSTP);
            }
        }
    }

    close(client_sockfd);  // Close the connection with the client
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;

    // Set up SIGINT handler for Ctrl-C
    signal(SIGINT, handle_sigint);

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }

    // Allow reuse of the port
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // Set up the server address struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the server address and port
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    listen(sockfd, 5);
    printf("Server listening on port %d...\n", PORT);

    while (1) {
        // Accept incoming connections
        client_len = sizeof(client_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (newsockfd < 0) {
            perror("Error accepting connection");
            continue;
        }

        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));

        // Fork a new process to handle the client
        if (fork() == 0) {
            // In the child process
            close(sockfd);  // Close the server socket in the child
            handle_client(newsockfd);  // Handle the client
            exit(0);
        } else {
            // In the parent process
            close(newsockfd);  // Close the client socket in the parent
        }
    }

    // Close the server socket
    close(sockfd);
    return 0;
}
