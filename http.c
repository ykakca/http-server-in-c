#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "8080"
#define BACKLOG 10

char* parse_get_req(char *buf) 
{
    size_t buf_len = strlen(buf);
    char req[4], *req_file;
    
    strncpy(buf, req, 3);
    req[3] = '\0';

    if (strcmp(req, "GET") != 0) {
        perror("http: invalid request");
        return -1;
    } 

    req_file = (char*)malloc(sizeof(char) * (buf_len-3));
    memcpy(req_file, buf+3, buf_len-3);

    return req_file;
}

void* get_in_addr(struct sockaddr *sa) 
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void sigchld_handler(int s) 
{
    int saved_errno = errno;
    
    while(waitpid(-1, NULL, WNOHANG));
    
    errno = saved_errno;
}

int main()
{
    int ai_ret, yes = 1;
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *ai_ptr; 
    struct sigaction sa;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];
    char msg[]

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((ai_ret = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s", gai_strerror(ai_ret));
        exit(1);
    }

    for (ai_ptr = servinfo; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next) {
        if ((sockfd = socket(ai_ptr->ai_family, ai_ptr->ai_socktype, 
                ai_ptr->ai_protocol)) == -1) {
            perror("http: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, 
            sizeof(int)) == -1) {
            perror("setsockopt");
            continue;
        }

        if (bind(sockfd, ai_ptr->ai_addr, ai_ptr->ai_addrlen)) {
            close(sockfd);
            perror("http: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (ai_ptr == NULL) {
        fprintf(stderr, "http: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connection...\n");

    while (1) {
        sin_size = sizeof(their_addr);
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, 
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof(s));
        printf("http: got connection from %s\n", s);
    }

    if (!fork()) {
        close(sockfd);
        recv
    }

}