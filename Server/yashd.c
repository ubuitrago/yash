#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include "serveTCPsocks.c"

extern int errno;

#define PATHMAX 255
static char u_server_path[PATHMAX+1] = "/tmp/";  /* default */
static char u_socket_path[PATHMAX+1];
static char u_log_path[PATHMAX+1];
static char u_pid_path[PATHMAX+1];

void sig_pipe(int n) {
   perror("Broken pipe signal");
}

/**
 * @brief Handler for SIGCHLD signal 
 */
void sig_chld(int n){
  int status;
  fprintf(stderr, "Child terminated\n");
  wait(&status); /* So no zombies */
}

/**
 * @brief Initializes the current program as a daemon, by changing working 
 *  directory, umask, and eliminating control terminal,
 *  setting signal handlers, saving pid, making sure that only
 *  one daemon is running. Modified from R.Stevens.
 * @param[in] path is where the daemon eventually operates
 * @param[in] mask is the umask typically set to 0
 */
void daemon_init(const char * const path, uint mask){
  pid_t pid;
  char buff[256];
  static FILE *log; /* for the log */
  int fd;
  int k;

  /* put server in background (with init as parent) */
  if ( ( pid = fork() ) < 0 ) {
    perror("daemon_init: cannot fork");
    exit(0);
  } else if (pid > 0) /* The parent */
    exit(0);

  /* the child */
  serve_inet_socket();
  /* Close all file descriptors that are open */
  for (k = getdtablesize()-1; k>0; k--)
      close(k);

  /* Redirecting stdin and stdout to /dev/null */
  if ( (fd = open("/dev/null", O_RDWR)) < 0) {
    perror("Open");
    exit(0);
  }
  dup2(fd, STDIN_FILENO);      /* detach stdin */
  dup2(fd, STDOUT_FILENO);     /* detach stdout */
  close (fd);
  /* From this point on printf and scanf have no effect */

  /* Redirecting stderr to u_log_path */
  log = fopen(u_log_path, "aw"); /* attach stderr to u_log_path */
  fd = fileno(log);  /* obtain file descriptor of the log */
  dup2(fd, STDERR_FILENO);
  close (fd);
  /* From this point on printing to stderr will go to /tmp/u-echod.log */

  /* Establish handlers for signals */
  if ( signal(SIGCHLD, sig_chld) < 0 ) {
    perror("Signal SIGCHLD");
    exit(1);
  }
  if ( signal(SIGPIPE, sig_pipe) < 0 ) {
    perror("Signal SIGPIPE");
    exit(1);
  }

  /* Change directory to specified directory */
  chdir(path); 

  /* Set umask to mask (usually 0) */
  umask(mask); 
  
  /* Detach controlling terminal by becoming sesion leader */
  setsid();

  /* Put self in a new process group */
  pid = getpid();
  setpgrp(); /* GPI: modified for linux */

  /* Make sure only one server is running */
  if ( ( k = open(u_pid_path, O_RDWR | O_CREAT, 0666) ) < 0 )
    exit(1);
  if ( lockf(k, F_TLOCK, 0) != 0)
    exit(0);

  /* Save server's pid without closing file (so lock remains)*/
  sprintf(buff, "%6d", pid);
  write(k, buff, strlen(buff));

  return;
}

int main(int argc, char  **argv){
  daemon_init(u_server_path, 0);
  return 0;
}