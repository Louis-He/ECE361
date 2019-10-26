#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <sys/time.h>

#define BACKLOG 10   // how many pending connections queue will hold


int main(int argc, char** argv) {
    if(argc != 2){
        printf("[Error] Not enough or too many arguments.\n");
        return (EXIT_FAILURE);
    }

    char* portNum = argv[1];

    // socket()
    struct addrinfo hints;
    struct addrinfo* res;
    int yes=1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    int rv = getaddrinfo(NULL, portNum, &hints, &res);

    int s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    bind(s, res->ai_addr, res->ai_addrlen);

    if(listen(s, BACKLOG) == -1){
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    // start transmission
    while (true) {
        /* code */
    }

    return 0;
}
