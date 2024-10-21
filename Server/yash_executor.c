#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h> 
#include "jobs.c"
#include "yash_executor.h"

#define DELIMITERS " \t\n"
#define MAX_TOKENS 200
#define FATHER getpid()

// Process variables
bool pipeline = false;
pid_t cpid = -1;
pid_t global_pid = -1;
pid_t left_cpid = -1;
pid_t right_cpid = -1;
char *dup_inString = NULL;

//Prototypes
void sig_handler(int signo);
void pipe_handler(int signo);

// Function to create a 2D array for input 
char **cmdParser(char *inString) {
    char **parsedcmd = malloc(MAX_TOKENS * sizeof(char *));
    if (parsedcmd == NULL) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Tokenize the command string
    char *token = strtok(inString, DELIMITERS);
    int i = 0;

    while (token != NULL && i < MAX_TOKENS - 1) {
        parsedcmd[i] = strdup(token);  // Duplicate the token to store it
        if (parsedcmd[i] == NULL) {
            printf("Memory allocation failed.\n");
            exit(EXIT_FAILURE);
        }
        i++;
        token = strtok(NULL, DELIMITERS);
    }

    parsedcmd[i] = NULL;  // Null-terminate the array of strings

    return parsedcmd;
}

// Linear Search of unsorted parsedcmd 2D-array
// Returns the index of the target
int linearSearch(char **parsedcmd, const char *target) {
    int i = 0;
    while (parsedcmd[i] != NULL) {
        if (strcmp(parsedcmd[i], target) == 0) {
            return i;
        }
        i++;
    }
    // target not found
    return -1; 
}

int handleOutputRedirection(char **parsedcmd, int stdout_rdt){
    int fd_rdt = open(parsedcmd[stdout_rdt + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd_rdt == -1 || dup2(fd_rdt, STDOUT_FILENO) == -1) {
        exit(EXIT_FAILURE);
    }
    parsedcmd[stdout_rdt] = NULL;
    close(fd_rdt); // Close file descriptor after duplicating
}

int handleInputRedirection(char **parsedcmd, int stdin_rdt){
    int fd_rdt = open(parsedcmd[stdin_rdt + 1], O_RDONLY | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd_rdt == -1 || dup2(fd_rdt, STDIN_FILENO) == -1) {
        exit(EXIT_FAILURE);
    }
    parsedcmd[stdin_rdt] = NULL;
    close(fd_rdt); // Close file descriptor after duplicating
}

void init_handler(){
    signal(SIGINT, sig_handler);
    signal(SIGTSTP, sig_handler);
}

void ignore_handler(){
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
}

int isForeground() {
    pid_t pid = tcgetpgrp(STDIN_FILENO);
    if(pid == -1 /* piped */ || pid == getpgrp() /* foreground */)
        return 1;
    /* otherwise background */
    return 0;
}

void sig_handler(int signo) {
    int fg = isForeground();
    //printf("FG %d\n", fg);
    //fflush(stdout);

    // Catches interrupt signal
    if(signo == SIGINT){
        //printf("SIGINT\n");
        if (pipeline){
            //printf("PARENT %d\n", getppid());
            //printf("KILL CHILD %d\n", cpid);
            killpg(getppid(), SIGINT);
        }else if (fg == 0)
            return;
        else
            //printf("PARENT %d\n", getppid());
            printf("KILL CHILD %d\n", cpid);
            kill(cpid, SIGINT);
    // Catches terminal stop         
    }else if (signo == SIGTSTP){
        if (pipeline){
            //printf("PARENT %d\n", getppid());
            printf("KILL CHILD %d\n", cpid);
            killpg(getppid(), SIGTSTP);
        }else if (fg == 0)
            return;
        else
            //printf("PARENT %d\n", getppid());
            //printf("KILL CHILD %d\n", cpid);
            kill(cpid, SIGTSTP);
            add_job((int) cpid, dup_inString, true); // Add to job table as stopped
    }
}

void pipe_handler(int signo){
    int fg = isForeground();
    printf("FG %d\n", fg);

    if (signo == SIGINT){
        //printf("SIGINT received, Interrupt pipeline group\n");
        //printf("LEFT CPID: %d\n",left_cpid);
        kill(left_cpid, SIGINT);
        kill(right_cpid, SIGINT);
    } else if (signo == SIGTSTP) {
        if (fg == 1){
            //printf("SIGTSTP received, killing pipeline group\n");
            killpg(left_cpid, SIGTSTP); // Send SIGTSTP to the entire process group
        }else
            return;
        
    }
}

void init_pipe_handler(){
    signal(SIGINT, pipe_handler);
    signal(SIGTSTP, pipe_handler);
}

int executePipeline(char **parsedcmd, int pipe_char){
    // Create special pipe handler in memory
    init_pipe_handler();
    // Init index of pipe character, left end, and right end arrays
    char *key = parsedcmd[0];
    char **left_end = malloc(MAX_TOKENS * sizeof(char *));
    char **right_end = malloc(MAX_TOKENS * sizeof(char *));
    
    // Split parsedcmd into left and right end of pipe for execvp to call seperately
    int i,j; 
    for (i = 0; i < pipe_char; ++i){
        left_end[i] = strdup(parsedcmd[i]);
    }
    i = 0; // re-init i to create right end image of pipe
    for (j = pipe_char+1; parsedcmd[j] != NULL; ++j){
        right_end[i] = strdup(parsedcmd[j]);
        ++i;
    }

    // Replace pipe character with NULL
    parsedcmd[pipe_char] = NULL;

    // Create pipe
    int p[2];
    if (pipe(p) < 0) {
        perror("Pipe creation failed");
        exit(EXIT_FAILURE);
    }
    // Fork left child
    left_cpid = fork();
    if (left_cpid == 0) {
        // if (setpgid(0, 0) < 0) {
        //     perror("Failed to set process group for left child");
        //     exit(EXIT_FAILURE);
        // }
        //printf("Left child PID: %d, PGID: %d\n", getpid(), getpgid(0));
        close(p[0]); // Close read end
        dup2(p[1], STDOUT_FILENO);
        close(p[1]);
        execvp(left_end[0], left_end);
        perror("Exec failed for left child");
        exit(EXIT_FAILURE);
    } else if (left_cpid < 0) {
        perror("Fork failed for left child");
        exit(EXIT_FAILURE);
    }

    // Fork right child
    right_cpid = fork();
    if (right_cpid == 0) {
        // if (setpgid(0, left_cpid) < 0) {
        //     perror("Failed to set process group for right child");
        //     exit(EXIT_FAILURE);
        // }
        //printf("Right child PID: %d, PGID: %d\n", getpid(), getpgid(0));
        close(p[1]); // Close write end
        dup2(p[0], STDIN_FILENO);
        close(p[0]);
        execvp(right_end[0], right_end);
        perror("Exec failed for right child");
        exit(EXIT_FAILURE);
    } else if (right_cpid < 0) {
        perror("Fork failed for right child");
        exit(EXIT_FAILURE);
    }

    //Close pipe ends in Parent
    close(p[0]);
    close(p[1]);

    //Debugging: Print process group info
    //printf("Parent PID: %d, Left PGID: %d, Right PGID: %d\n", getpid(), getpgid(left_cpid), getpgid(right_cpid));

    // Wait for both children to exit
    int status;
    waitpid(left_cpid, &status, WUNTRACED | WCONTINUED);
    waitpid(right_cpid, &status, WUNTRACED | WCONTINUED);

    // Free memory
    free(left_end);
    free(right_end);
    free(parsedcmd);

    return 0;
}

/*****************************************************************************************/
/* yash (Yet Another SHell) program entrypoint, written by Uriel A. Buitrago (uab62)
/ September 29, 2024. EE382V-Project1
/*****************************************************************************************/
// Program Launcher
int yash_entrypoint(char *inString, int pipefd[2], int psd){
    pid_t parent = getpid();
    cpid = -1;
    char **parsedcmd;
    int status;

    // Initialize signal handler in memory
    ignore_handler();
    // Process input string from user
    /*---------------------------------------------------------------------------------------*/
    //do {
    if (!inString || strcmp(inString, "\n") == 0 || strcmp(inString, "") == 0){
        // Print any DONE jobs
        print_jobs();
        free(inString);
        exit(0);
    }
    /*---------------------------------------------------------------------------------------*/
    // Copy original string for updating job table
    dup_inString = strdup(inString);
    // Create tokenized string & handle special char/str cases
    parsedcmd = cmdParser(inString);

    // CASE: STDOUT redirect
    int stdout_rdt = linearSearch(parsedcmd, ">");
    // CASE: STDIN redirect
    int stdin_rdt = linearSearch(parsedcmd, "<");
    // CASE: Pipeline
    int pipe_char = linearSearch(parsedcmd, "|");
    // CASE: & (background process)
    int bg_char = linearSearch(parsedcmd, "&");
    //CASE: jobs
    bool trigger_job_cmd = true ?/*IF*/linearSearch(parsedcmd, "jobs") >= 0 :/*ELSE*/ false;
    // CASE: fg
    bool trigger_fg_cmd = true ?/*IF*/linearSearch(parsedcmd, "fg") >= 0 :/*ELSE*/ false;
    // CASE: bg
    bool trigger_bg_cmd = true ?/*IF*/linearSearch(parsedcmd, "bg") >= 0 :/*ELSE*/ false;

    // Set process groups 
    if (setpgid(0,0) < 0){
        perror("setpgid");
        return -1;
    }
    /*---------------------------------------------------------------------------------------*/
    //Handle expected job control commands: jobs, fg, bg

    // Run job commands
    if (trigger_job_cmd){
        free(dup_inString);
        dup2(pipefd[1], STDOUT_FILENO);
        print_jobs();
        fflush(stdout);
        return 0;
    }else if (trigger_fg_cmd){  // Run fg command
        free(dup_inString);
        dup2(pipefd[1], STDOUT_FILENO);
        fg_job();
        return 0;
    }else if (trigger_bg_cmd){  // Run bg command
        free(dup_inString);
        dup2(pipefd[1], STDOUT_FILENO);
        bg_job();
        return 0;
    }
    /*---------------------------------------------------------------------------------------*/
    // Set global tracker for pipline procs
    if (pipe_char >= 0){
        pipeline = true;
    }

    // Initialize signal handlers
    init_handler();

    //Set & char to NULL
    if(bg_char >= 0){
        parsedcmd[bg_char] = NULL;
    }
    /*---------------------------------------------------------------------------------------*/
    // Disable buffering for stdout
    setvbuf(stdout, NULL, _IONBF, 0);
    // Set pipe to non-blocking mode
    int flags = fcntl(pipefd[0], F_GETFL, 0);  // Get the current file descriptor flags
    fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);  // Set the non-blocking flag
    // Fork child proc 
    cpid = fork();

    if (cpid > 0) { // Parent process
        // Waits if background flag NOT activated, i.e. foreground process
        if (bg_char < 0){

            /* Removed tcsetpgrp because of how it handles signals */
            // Bring process group to the foreground
            // if (tcsetpgrp(STDIN_FILENO, cpid) < 0) {
            //     perror("Failed to set terminal to foreground job");
            // }

            // Wait for the process
            if (waitpid(cpid, &status, WUNTRACED | WCONTINUED) < 0) {
                perror("Failed to wait for process");
                return -1;
            }
            printf("Status %d\n", status);
            if (status != 0)
                return -1;
            // Return terminal control to yash
            // if (tcsetpgrp(STDIN_FILENO, parent) < 0) {
            //     perror("Failed to return control to shell");
            // }
        }
        else{
            add_job((int) cpid, dup_inString, false); // Job starts as running
            waitpid(cpid, &status, WNOHANG);
            printf("& Status %d\n", status);
        }

        // Free dynamically allocated memory
        // free(dup_inString);
        // free(inString);
        // for (int i = 0; parsedcmd[i] != NULL; i++) {
        //     free(parsedcmd[i]);
        // }
        ignore_handler();
        pipeline = false;
        printf("FFLUSH");
        return 0;
    }
    
    else if (cpid == 0) { // Child process
        // Redirect child's STDOUT to the server's pipe (write end)
        dup2(pipefd[1], STDOUT_FILENO);
        //dup2(pipefd[0], STDIN_FILENO);
        // Redirect child's STDIN to the client socket
        dup2(psd, STDIN_FILENO);

        // Close the pipe descriptors in the child
        close(pipefd[1]);
        //close(pipefd[0]);

        // < 
        if (stdin_rdt >= 0){
            handleInputRedirection(parsedcmd, stdin_rdt);
        }
        // >
        if (stdout_rdt >= 0) {
            handleOutputRedirection(parsedcmd, stdout_rdt);
        }
        // | 
        if (pipe_char >= 0){
            if (executePipeline(parsedcmd, pipe_char) != 0){
                perror("Pipeline error");
            }
            ignore_handler();
            exit(0); 
        }

        // Execute new image   
        if (execvp(parsedcmd[0], parsedcmd) < 0){
            //printf("parsedcmd %s", parsedcmd);
            exit(EXIT_FAILURE);
        }
        printf("Child ran %s", parsedcmd);
        fflush(stdout);
        // Free allocated memory
        //free(parsedcmd);
        //kill(0, SIGCHLD);
        //return 0;
        exit(0);
    } else 
        perror("fork failed"); // Error if fork fails

    //} while (inString != NULL);

    //exit(0);
}