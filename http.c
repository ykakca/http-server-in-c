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
#define RESPONSE_STATUS_404 "HTTP/1.1 404 Not Found\r\n"
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
    char *temp1, *temp2, *temp3;

    temp1 = strchr(buf, ' ');

    req->req_type = malloc(sizeof(char) * (temp1-buf+1));
    strncpy(req->req_type, buf, temp1-buf);
    req->req_type[temp1-buf] = '\0';

    //sonradan degistirilecek
    if (strcmp(req->req_type, "GET") != 0) {
        perror("http: invalid request");
        free(req->req_type);
        free(req);
        return NULL;
    } 

    temp2 = strchr(temp1+1, ' ');

    req->path = malloc(sizeof(char) * (temp2-temp1+1));
    strncpy(req->path, temp1+1, temp2-temp1);
    req->path[temp2-temp1] = '\0';

    temp3 = strchr(temp2+1, ' ');

    req->version = malloc(sizeof(char) * (temp3-temp2+1));
    strncpy(req->version, temp2+1, temp3-temp2);
    char* crlf = strchr(req->version, '\r');
    if (crlf) *crlf = '\0';
    req->version[temp3-buf] = '\0';

    printf("\n\n");
    printf("\ntype: %s*", req->req_type);
    printf("\npath: %s*", req->path);
    printf("\nversion: %s*", req->version);
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
        buf->data[len] = '/0';
        if (ferror(fptr)) {
            fprintf(stderr, "error while reading file %s", filename);
            return NULL;
        }

        buf->size = new_len;
    }
    
    fclose(fptr);
    return buf;
}


char* prepare_http_response(char* filename) 
{
    int status;
    int bytes_written = 0;
    size_t response_size;
    char* response;
    char* status_str;
    BufferedFile* buf;
    char* file_to_read = filename;

    if (filename == NULL) {
        perror("filename not specified");
        return NULL;
    }

    if (filename[0] == '/') {
        status = 200;
        file_to_read = "index.html";
    }

    else if (fopen(filename, "r") == NULL) {
        status = 404;
    }
    else {
        status = 200;
    }

    if (status == 200) {
        status_str = RESPONSE_STATUS_200;
        buf = read_file_into_buf(file_to_read);
    }
    else if (status == 404) {
        status_str = RESPONSE_STATUS_404;
        buf = read_file_into_buf("404.html");
    }
        
    response_size = buf->size + MAX_STATUS_LEN;
    response = (char*)malloc(response_size*sizeof(char));

    bytes_written += snprintf(response, response_size, status_str);
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
    // ParsedHTTPReq *req = parse_get_req(reqbuf);

    // free(req->path);
    // free(req->version);
    // free(req->req_type);
    // free(req);
    ParsedHTTPReq *req = parse_get_req(reqbuf);

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
            recv_buf[bytes_received-1] = '\0';
            // printf("\nreceived:\n %s", recv_buf);

            req = parse_get_req(recv_buf);
        }

        if (req == NULL) {
            perror("null request");
            continue;
        }

        response = prepare_http_response(req->path+1);

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
    free(req->path);
    free(req->req_type);
    free(req->version);
    free(req);
    // free(buf->data);
    // free(buf);
}