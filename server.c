#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "3490"
#define BACKLOG 10

//make sure child processes don't turn into zombies
void sigchld_handler(int s)
{
    (void) s;
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);
    
    errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa) 
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr); 
}