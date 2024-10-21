#define DELIMITERS " \t\n"
#define MAX_TOKENS 200
#define FATHER getpid()
#include "jobs.h"
//Prototypes
void sig_handler(int signo);
void pipe_handler(int signo);

// Function to create a 2D array for input 
char **cmdParser(char *inString);

// Linear Search of unsorted parsedcmd 2D-array
// Returns the index of the target
int linearSearch(char **parsedcmd, const char *target);

int handleOutputRedirection(char **parsedcmd, int stdout_rdt);

int handleInputRedirection(char **parsedcmd, int stdin_rdt);

// Executes pipelines 
int executePipeline(char **parsedcmd, int pipe_char);

// Entrypoint
int yash_entrypoint(char *inString, int pipefd[2], int psd);