#ifndef JOBS_H
#define JOBS_H
#include <sys/types.h>
#include <stdbool.h>
// Maximum number of jobs
#define MAX_JOBS 20

// Enum for job status
typedef enum { RUNNING, STOPPED, DONE } job_status_t;

// Structure to represent a job
typedef struct {
    int job_num;             // The job number
    pid_t pid;               // Process ID of the job
    job_status_t status;     // Status of the job (RUNNING, STOPPED, DONE)
    char command[256];       // Command string associated with the job
} job_t;

// Declare global variables with extern to avoid multiple definitions
extern job_t jobs[MAX_JOBS]; // Array to hold job structures
extern int job_count;        // Number of current jobs
extern int current_job;      // Index of the current job (marked with +)

// Function prototypes
// Add a job to the job table
void add_job(pid_t pid, char *command, bool stopped);
// Remove job
void remove_job(int job_idx);
// Update the status of jobs based on signals
void update_job_status();
// Print jobs in the required format
void print_jobs();
// Foreground stopped Job
void fg_job();
// Background job
void bg_job();

#endif