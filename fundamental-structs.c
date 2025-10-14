#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//Fundamental structs

struct addrinfo {                       // Address info, used generally as a linked list,
    int             ai_flags;           // getaddrinfo() function returns a linked list of 
    int             ai_family;          // these structs filled out
    int             ai_socktype;
    int             ai_protocol;
    size_t          ai_addrlen;
    struct sockaddr *ai_addr;
    char            *ai_canonname;

    struct addrinfo *ai_next;
};

struct sockaddr {
    unsigned short  sa_family;          // Holds Socket IP address info 
    char            sa_data[14];
};

struct sockaddr_in {
    short int           sin_family;     // Used to deal with the sockaddr struct,
    unsigned short int  sin_port;       // this one is more practical because this one
    struct in_addr      sin_addr;       // makes it easy to reference elements of the
    unsigned char       sin_zero[8];    // reference elements of the socket address
};                                      // this struct can be easily cast into struct sockaddr

struct in_addr {                        // Used inside the sockaddr_in struct
    uint32_t s_addr;                    // The IPv4 address is stored as a 32 bit int
};

struct sockaddr_in6 {                   // The IPv6 version of the sockaddr_in struct 
    u_int16_t       sin6_family;    
    u_int16_t       sin6_port;
    u_int32_t       sin6_flowinfo;
    struct in6_addr sin6_addr;
    u_int32_t       sin6_scope_id;
};

struct in6_addr {                       // The IPv6 version of the in_addr struct 
    unsigned char   s6_addr[16];
};

struct sockaddr_storage {               // Can hold both IPv4 and IPv6
    sa_family_t ss_family; // address family
    // all this is padding, implementation specific, ignore it:
    char __ss_pad1[_SS_PAD1SIZE];
    int64_t __ss_align;
    char __ss_pad2[_SS_PAD2SIZE]
};