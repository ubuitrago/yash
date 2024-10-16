#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "jobs.h"

// Job control table
job_t jobs[MAX_JOBS];      // Array of job structures
int job_count = 0;         // Count of current jobs
int current_job = 0;       // Index of the current job (+)

// Add a job to the job table
void add_job(pid_t pid, char *command, bool stopped) {
    if (job_count >= MAX_JOBS) {
        printf("Maximum number of jobs reached.\n");
        return;
    }
    jobs[job_count].job_num = job_count + 1;
    jobs[job_count].pid = pid;
    jobs[job_count].status = stopped ? STOPPED : RUNNING; // New jobs start as running OR stopped
    strncpy(jobs[job_count].command, command, 255);
    jobs[job_count].command[255] = '\0'; // Ensure null-termination
    current_job = job_count; // Set this as the current job (+)
    job_count++;
}

// Remove a job from the job table
void remove_job(int job_idx) {
    for (int j = job_idx; j < job_count - 1; j++) {
        jobs[j] = jobs[j + 1];  // Shift jobs in the table
    }
    job_count--;  // Decrease the job count
    current_job = job_count - 1;  // Update the current job index
}

// Update the status of jobs based on signals
void update_job_status() {
    for (int i = 0; i < job_count; i++) {
        int status;
        pid_t result = waitpid(jobs[i].pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        //printf("RESULT: %d", result);
        if (result == 0) {
            // Process is still running, do nothing
            continue;
        }else if (result == -1) {
            // Error checking status
            perror("waitpid");
            continue;
        }

        // Check specific status codes
        if (WIFSTOPPED(status)) {
            jobs[i].status = STOPPED;
        } else if (WIFCONTINUED(status)) {
            jobs[i].status = RUNNING;
        } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
            jobs[i].status = DONE;
        }
    }
}

void print_jobs() {
    update_job_status(); // Update the status of jobs before printing
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].status == DONE) {
            // Print completed jobs
            printf("[%d]   Done       %s\n", jobs[i].job_num, jobs[i].command);
            // Remove the job after printing
            remove_job(i);
            i--; // Adjust index after removing the job
        } else {
            // Determine the current job indicator (+ or -)
            char indicator = (i == current_job) ? '+' : '-';
            // Print running or stopped jobs
            printf("[%d] %c %s   %s\n", jobs[i].job_num, indicator,
                   jobs[i].status == RUNNING ? "Running" : "Stopped",
                   jobs[i].command);
        }
    }
}


void fg_job() {

    // Ignore SIGTTOU in the shell to prevent the shell from being stopped
    signal(SIGTTOU, SIG_IGN);

    // Get the most recent job
    int fg_job_idx = job_count - 1;
    pid_t fg_job_pid = jobs[fg_job_idx].pid;

    //Set the terminal's process group to the job's process group
    if (tcsetpgrp(STDIN_FILENO, fg_job_pid) < 0) {
        perror("Failed to set process group to job");
        return;
    }

    // Send SIGCONT to the most recent background or stopped process
    if (kill(fg_job_pid, SIGCONT) < 0) {
        perror("Failed to send SIGCONT");
        return;
    }

    // Print the process name to stdout
    printf("%s\n", jobs[fg_job_idx].command);

    // Wait for the process to either stop or complete
    int status;
    if (waitpid(fg_job_pid, &status, WUNTRACED | WCONTINUED) < 0) {
        perror("Failed to wait for process");
        return;
    }

    // Process was stopped, update the job status
    if (WIFSTOPPED(status)) {
        jobs[fg_job_idx].status = STOPPED;
    }
    // Process exited or was terminated, remove it from the job table
    else if (WIFEXITED(status) || WIFSIGNALED(status)) {
        jobs[fg_job_idx].status = DONE;
        // Remove the job from the job table
        remove_job(fg_job_idx);
    }

    // Return terminal control to yash
    if (tcsetpgrp(STDIN_FILENO, getpid()) < 0) {
        perror("Failed to set process group back to shell");
    }

    // Restore to default
    signal(SIGTTOU, SIG_DFL);
}

void bg_job() {
    // Check if there are jobs available
    if (job_count == 0) {
        printf("No jobs to bring to the background.\n");
        return;
    }

    // Get the most recent job that was stopped
    int bg_job_idx = job_count - 1;
    pid_t bg_job_pid = jobs[bg_job_idx].pid;

    // Ensure the job is currently stopped
    if (jobs[bg_job_idx].status != STOPPED) {
        printf("Job is not stopped.\n");
        return;
    }

    // Send SIGCONT to the process to resume it in the background
    if (kill(bg_job_pid, SIGCONT) < 0) {
        perror("Failed to send SIGCONT to background job");
        return;
    }

    // Update job status to running
    jobs[bg_job_idx].status = RUNNING;

    printf("[%d] %s &\n", jobs[bg_job_idx].job_num, jobs[bg_job_idx].command);
}

