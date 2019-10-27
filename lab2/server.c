#define _POSIX_C_SOURCE 200112L
#define MAXDATASIZE 1000

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <sys/time.h>

#include "clientInfo.h"
#include "server.h"
#include "message.h"

#define BACKLOG 10   // how many pending connections queue will hold

extern struct clientInfo currentClientInfo[MAX_USER];
extern struct sessionInfo currentSessionInfo[MAX_CONN];

/*  * Part of the code is cited from https://beej.us/guide/bgnet/  */

int main(int argc, char** argv) {
    if(argc != 2){
        printf("[Error] Not enough or too many arguments.\n");
        return (EXIT_FAILURE);
    }

    initializeRecord();
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
    int new_fd;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    bind(s, res->ai_addr, res->ai_addrlen);

    if(listen(s, BACKLOG) == -1){
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    // start transmission
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;

    char buf[MAXDATASIZE];
    while (true) {
        sin_size = sizeof their_addr;
        new_fd = accept(s, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        // ?? mutithreading?
        if (!fork()) { // this is the child process
            close(s); // child doesn't need the listener

            int numbytes;
            if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
                perror("recv");
                exit(1);
            }
            buf[numbytes] = '\0';
            printf("Server: received '%s'\n",buf);
            processIncomingMsg(buf);

            if (send(new_fd, "Hello, world!", 13, 0) == -1)
                perror("send");
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }

    return 0;
}

void processIncomingMsg(char* incomingMsg){
    int msgType;
    int msgSize;
    unsigned char msgSourceAndData[MAXDATASIZE];
    unsigned char msgSource[MAXDATASIZE];
    unsigned char msgData[MAXDATASIZE];
    sscanf(incomingMsg, "%d,%d,%s", &msgType, &msgSize, msgSourceAndData);

    // split soruce and data
    char* comma;
    comma = strchr ((char*) msgSourceAndData, ',');
    *comma = '\0';
    comma += sizeof(unsigned char);
    strcpy((char*) msgSource, (char*) msgSourceAndData);
    strcpy((char*) msgData, (char*) comma);
    // processing over
    // msgType, msgSize, msgSource, msgData are loaded and wait for processing

    if(msgType == 1){
        // seperate username and password
        unsigned char userName[MAXDATASIZE];
        unsigned char userPW[MAXDATASIZE];

        char* colon;
        colon = strchr ((char*) msgData, ':');
        *colon = '\0';
        colon += sizeof(unsigned char);
        strcpy((char*) userName, (char*) msgData);
        strcpy((char*) userPW, (char*) colon);

        // start login in process
        unsigned char loginBackMesssage[MAXDATASIZE];
        bool isLoginSuccessful = attemptLogin(userName, userPW, loginBackMesssage);
        printf("%d, %s\n", isLoginSuccessful, loginBackMesssage);
    }
}
