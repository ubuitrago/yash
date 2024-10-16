/**
 * @file TCPServer-ex2.c 
 * @brief The program creates a TCP socket in
 * the inet domain and listens for connections from TCPClients, accept clients
 * into private sockets, and fork an echo process to ``serve'' the
 * client.  If [port] is not specified, the program uses any available
 * port.  
 * Run as: 
 *     TCPServer-ex2 <port>
 */
/*
NAME:        
SYNOPSIS:    TCPServer [port]

DESCRIPTION:  

*/
#include <stdio.h>
/* socket(), bind(), recv, send */
#include <sys/types.h>
#include <sys/socket.h> /* sockaddr_in */
#include <netinet/in.h> /* inet_addr() */
#include <arpa/inet.h>
#include <netdb.h> /* struct hostent */
#include <string.h> /* memset() */
#include <unistd.h> /* close() */
#include <stdlib.h> /* exit() */
#include <pthread.h>
#include "serveTCPsocks.h"
#include "yash_executor.c"

#define BUFSIZE 512
int serve_inet_socket() {
    int   sd, psd;
    struct   sockaddr_in server;
    struct ClInfo client;
    struct  hostent *hp, *gethostbyname();
    struct sockaddr_in from;
    int fromlen;
    int length;
    char ThisHost[MAXHOSTNAME];
    int pn;
    int childpid;
    pthread_t th[MAXCONNECTIONS];
    
    /* get TCPServer1 Host information, NAME and INET ADDRESS */
    gethostname(ThisHost, MAXHOSTNAME);
    /* OR strcpy(ThisHost,"localhost"); */
    
    fprintf(stderr,"----TCP/Server running at host NAME: %s\n", ThisHost);
    if  ( (hp = gethostbyname(ThisHost)) == NULL ) {
      fprintf(stderr, "Can't find host\n");
      exit(-1);
    }
    bcopy ( hp->h_addr, &(server.sin_addr), hp->h_length);
    fprintf(stderr,"    (TCP/Server INET ADDRESS is: %s )\n", inet_ntoa(server.sin_addr));
    /** Construct name of socket */
    server.sin_family = AF_INET;
    /* OR server.sin_family = hp->h_addrtype; */
    
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(3820);
    // if (argc == 1)
    //     server.sin_port = htons(3820);  
    // else  {
    //     pn = htons(atoi(argv[1])); 
    //     server.sin_port =  pn;
    // }
    
    /** Create socket on which to send  and receive */
    
    sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
    /* OR sd = socket (hp->h_addrtype,SOCK_STREAM,0); */
    if (sd<0) {
	perror("opening stream socket");
	exit(-1);
    }
    /** this allow the server to re-start quickly instead of waiting
	for TIME_WAIT which can be as large as 2 minutes */
    reusePort(sd);
    /* Assign a name to the created socket*/
    if ( bind( sd, (struct sockaddr *) &server, sizeof(server) ) < 0 ) {
	close(sd);
	perror("binding name to stream socket");
	exit(-1);
    }
    
    /** get port information and  prints it out */
    length = sizeof(server);
    if ( getsockname (sd, (struct sockaddr *)&server,&length) ) {
	perror("getting socket name");
	exit(0);
    }
    fprintf(stderr,"Server Port is: %d\n", ntohs(server.sin_port));
    
    /** accept TCP connections from clients and create a thread to serve each */
    listen(sd,MAXCONNECTIONS);
    fromlen = sizeof(from);
    for(int i=0; i < MAXCONNECTIONS; i++){
        psd  = accept(sd, (struct sockaddr *)&from, &fromlen);
        client.sock = psd;
        client.from = from;
        pthread_create(&th[i], NULL, serve_yash, (void *) &client);
    }
}
//TODO:Method that creates a message queue for each serve_yash thread

void *serve_yash(void * input) {
    char buf[BUFSIZE];
    char command[BUFSIZE];
    int rc;
    int psd;
    struct sockaddr_in from;

    struct ClInfo *info = (struct ClInfo *)input;
    psd = info->sock;
    from = info->from; 
	
    struct  hostent *hp, *gethostbyname();

    // Save original stdin and stdout
    int saved_stdin = dup(STDIN_FILENO);
    int saved_stdout = dup(STDOUT_FILENO);
    
    printf("Serving %s:%d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));

    if ((hp = gethostbyaddr((char *)&from.sin_addr.s_addr,
			    sizeof(from.sin_addr.s_addr),AF_INET)) == NULL)
	fprintf(stderr, "Can't find host %s\n", inet_ntoa(from.sin_addr));
    else
	printf("(Name is : %s)\n", hp->h_name);
    
    // Get data from  client
    int exec_return; // Return value from exec image within serving thread
    for(;;){
    	//printf("\n...server is waiting...\n");
    	if((rc=recv(psd, buf, sizeof(buf), 0)) < 0){
    	    perror("receiving stream  message");
    	    exit(-1);
    	}

        // Parse received buffer
    	if (rc > 0){
    	    buf[rc]='\0';
    	    printf("Received: %s\n", buf);
    	    //printf("From TCP/Client: %s:%d\n", inet_ntoa(from.sin_addr),
    		   //ntohs(from.sin_port));
    	    //printf("(Name is : %s)\n", hp->h_name);
            // Handle CMD messages
            if (strncmp(buf, "CMD ", 4) == 0) {
                strncpy(command, buf + 4, sizeof(command)-1);  // Extract the command after "CMD "
                command[sizeof(command) - 1] = '\0';  // Remove newline character

                printf("Executing command: %s\n", command);

                // Temporarily redirect stdout to the client socket
                dup2(psd, STDOUT_FILENO);
                dup2(psd, STDERR_FILENO);
                // send command to yash (project 1) entrypoint
                exec_return = yash_entrypoint(command);
                // Restore original stdin/stdout
                dup2(saved_stdin, STDIN_FILENO);   
                dup2(saved_stdout, STDOUT_FILENO);
                if (exec_return == 0) {   
                    // After command execution, send the prompt to the client      
                    if (send(psd, "\n# ", 3, 0) <0 )
                      perror("sending stream message");
                } else {
                    printf("Return value: %d\n", exec_return);
                }
            }

            // After command execution, send the prompt to the client      
    	    // if (send(psd, "\n# ", 3, 0) <0 )
    		//   perror("sending stream message");
    	}
    	else {
    	    printf("TCP/Client: %s:%d\n", inet_ntoa(from.sin_addr),
    		   ntohs(from.sin_port));
    	    printf("(Name is : %s)\n", hp->h_name);
    	    printf("Disconnected..\n");
    	    close (psd);
    	    pthread_exit(0);
        }
    }
}

void reusePort(int s)
{
    int one=1;
    
    if ( setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(char *) &one,sizeof(one)) == -1 )
	{
	    printf("error in setsockopt,SO_REUSEPORT \n");
	    exit(-1);
	}
}      
