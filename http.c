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
#define RESPONSE_STATUS_200 "HTTP/1.1 200 OK\r\n"
#define MAX_STATUS_LEN 50

typedef struct BufferedFile {
    char* data;
    size_t size;
} BufferedFile;

typedef struct ParsedHTTPReq {
    char* req_type;
    char* path;
    char* version;
} ParsedHTTPReq;

ParsedHTTPReq* parse_get_req(char *buf) 
{
    ParsedHTTPReq *req = malloc(sizeof(ParsedHTTPReq));
    size_t buf_len = strlen(buf);
    char req_type[4], *req_file, *path, temp;
    int i = 0, j = 0;

    temp = buf[0];
    while (temp != ' ' && temp != '\0') {
        temp = buf[i];
        i++;
    }

    req->req_type = malloc(sizeof(char) * i);
    strncpy(req->req_type, buf, i);
    req->req_type[i-1] = '\0';

    //sonradan degistirilecek
    if (strcmp(req->req_type, "GET") != 0) {
        perror("http: invalid request");
        return NULL;
    } 

    temp = buf[i];
    while (temp != ' ' && temp != '\0') {
        temp = buf[i+j];
        j++;
    }

    // path = (char*)malloc(sizeof(char) * j);
    req->path = malloc(sizeof(char) * j);
    strncpy(req->path, buf+i, j);
    req->path[j-1] = '\0';

    printf("\n\n%c\n\n", buf[i+j]);

    printf("i=%d j=%d buf[i+j]=%c\n", i, j, buf[i+j]);

    req->version = malloc(sizeof(char) * (buf_len-i));
    strncpy(req->version, buf+i+j, buf_len-(i+j));
    char* crlf = strchr(req->version, '\r');
    if (crlf) *crlf = '\0';
    req->version[buf_len-(i+j)-1] = '\0';

    printf("\n%slen:%d", req->req_type, i);
    printf("\n%slen:%d", req->path, j);
    printf("\n%slen:%ld", req->version, buf_len-(i+j));
    printf("\n\n");

    return req;
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

BufferedFile* read_file_into_buf(char* filename)
{
    FILE* fptr;
    BufferedFile* buf = (BufferedFile*)malloc(sizeof(BufferedFile));
    size_t len;

    if (filename == NULL) {
        perror("File name cannot be empty!");
        return NULL;
    }

    fptr = fopen(filename, "r");
    if (fptr == NULL) {
        fprintf(stderr, "file %s could not be opened", filename);
        return NULL;
    }

    if (fseek(fptr, 0L, SEEK_END) == 0) {
        len = ftell(fptr);
        if (len == -1) {
            fprintf(stderr, "error while reading file %s", filename);
            return NULL;
        }
        buf->data = (char*)malloc((len+1) * sizeof(char));

        if (fseek(fptr, 0L, SEEK_SET) != 0) {
            fprintf(stderr, "error while reading file %s", filename);
            return NULL;
        }; 

        size_t new_len = fread(buf->data, sizeof(char), len, fptr);
        if (ferror(fptr)) {
            fprintf(stderr, "error while reading file %s", filename);
            return NULL;
        }

        buf->size = new_len;
    }

    // printf("\nbuffer size: %ld", buf->size);
    // printf("\nbuffered data : %s", buf->data);
    
    fclose(fptr);
    return buf;
}

char* prepare_http_response(int status, char* filename) 
{
    int bytes_written = 0;
    size_t response_size;
    char* response;
    BufferedFile* buf;

    if (status == 200) {
        if (filename == NULL) {
            buf = (BufferedFile*)malloc(sizeof(BufferedFile));
            buf->size = 0;
            buf->data = NULL;
        }
        else {
            buf = read_file_into_buf(filename);
        }

        response_size = buf->size + MAX_STATUS_LEN;
        response = (char*)malloc(response_size*sizeof(char));

        bytes_written += snprintf(response, response_size, RESPONSE_STATUS_200);
        bytes_written += snprintf(response+bytes_written, response_size-bytes_written, "Content-Length: %ld\r\n", buf->size);

        if (buf->size > 0) {
            bytes_written += snprintf(response+bytes_written, response_size-bytes_written, "\r\n");
            bytes_written += snprintf(response+bytes_written, response_size-bytes_written, buf->data);
        }

        printf("\nRESPONSE: \n%s", response);

        if (buf->size > 0) {
            free(buf->data);
        }
        free(buf);
    }

    return response;
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

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // BufferedFile* buf = read_file_into_buf("index.html");

    char* reqbuf = "GET /index.html HTTP/1.1\r\n";
    ParsedHTTPReq *req = parse_get_req(reqbuf);

    free(req->path);
    free(req->version);
    free(req->req_type);
    free(req);

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

    char recv_buf[1024];
    // char send_buf[1024] = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
    char* response;

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

        int bytes_received, bytes_sent;
        
        bytes_received = recv(new_fd, recv_buf, sizeof(recv_buf), 0);
        if (bytes_received == -1) {
            perror("recv");
            close(new_fd);
            continue;
        }
        else {
            recv_buf[bytes_received] = '\0';
            printf("\nreceived: %s", recv_buf);
        }

        response = prepare_http_response(200, "index.html");

        bytes_sent = send(new_fd, response, strlen(response), 0);
        if (bytes_sent == -1) {
            perror("send");
            // close(new_fd);
            continue;
        }
        close(new_fd);

        inet_ntop(their_addr.ss_family, 
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof(s));
        printf("http: got connection from %s\n", s);
    }

    free(response);
    // free(buf->data);
    // free(buf);
}