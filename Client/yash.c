// This is the Client implementation. 

/*
1- Connect to Server: 
    The client connects to the server using the server's 
    IP address and port 3820.
2- Send Command: 
    When the user types a command, the client sends the command 
    to the server, prefixed by CMD.

3- Receive Response: 
    The client waits for the response from the server, 
    displaying the result of the command and
    the prompt # sent by the server.
4- Signal Handling:
    Ctrl-C: Send a control message CTL c\n to 
         interrupt the current running command.
    Ctrl-Z: Send a control message CTL z\n to 
        suspend the current running command.
    Ctrl-D (EOF): Terminate the client connection 
        or allow the user to type quit to exit.
5- Protocol: 
    The client adheres to the specified protocol 
    for sending and receiving messages from the server.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 3820
#define BUFSIZE 1024

int sockfd;  // Global socket descriptor
char buffer[BUFSIZE];

// Signal handler for SIGINT (Ctrl-C)
void handle_sigint(int sig) {
    // Send control message to the server to stop the current command
    char ctl_msg[] = "CTL c\n";
    send(sockfd, ctl_msg, strlen(ctl_msg), 0);
}

// Signal handler for SIGTSTP (Ctrl-Z)
void handle_sigtstp(int sig) {
    // Send control message to the server to suspend the current command
    char ctl_msg[] = "CTL z\n";
    send(sockfd, ctl_msg, strlen(ctl_msg), 0);
}

void clean_buffer(char *buffer){
    memset(buffer, 0 , BUFSIZE);
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

int contains_gt(const char *input) {
    // Check if the input string contains the character '>'
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] == '>') {
            return 1;  // Found '>', return true
        }
    }
    return 0;  // '>' not found, return false
}

void handle_client(int sockfd) {
    char input[BUFSIZE];
    int rc;
    printf("# ");  // Display prompt
    // Infinite loop to handle client commands
    while (1) {
        // Free memory 
        memset(input, 0, BUFSIZE);
        // Read user input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;  // Exit if EOF (Ctrl-D)
        }
        // Remove trailing newline
        input[strcspn(input, "\n")] = 0;

        // Check if the input is empty (Enter key, or only spaces/tabs)
        if (is_empty_input(input)) {
            printf("# ");  // If input is empty or whitespace, print the prompt again
            continue;      // Skip sending to the server
        }
        
        // Check if output is redirected
        //int output_redirect = contains_gt(input);

        // Handle "quit" to exit the client
        if (strcmp(input, "quit") == 0) {
            break;
        }

        // Send the command to the server
        snprintf(buffer, sizeof(buffer), "CMD %s\n", input);
        send(sockfd, buffer, strlen(buffer), 0);

        // Receive the response from the server
        clean_buffer(buffer); // Zero-out buffer
      
        rc = recv(sockfd, buffer, BUFSIZE, 0);
        if (rc > 0) {
            // Ensure the prompt is null-terminated
            buffer[rc] = '\0';
            // Search for the last occurrence of '#' in the buffer
            //pound_prompt = strrchr(buffer, '#');
        
        
            // Display the server's response
            printf("%s", buffer);

            // clen buffer and prepare for next command
            clean_buffer(buffer);
            // rc = recv(sockfd, buffer, BUFSIZE, 0);
            // // Ensure the prompt is null-terminated
            // buffer[rc] = '\0';
            // printf("%s", buffer);
            continue;
            // Continue if output is not expected
            // if (output_redirect)
            //     continue;
            // else    // Else retrieve the prompt sent by server
            //     rc = recv(sockfd, buffer, BUFSIZE, 0);
            //     printf("%s", buffer);
            //     continue;
        } else if (rc == 0) {
            printf("Server disconnected.\n");
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;

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

    // Handle the client operations
    handle_client(sockfd);

    // Close the socket
    close(sockfd);
    return 0;
}
