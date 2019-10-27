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

            bool isCloseConn = false;
            while(!isCloseConn){
                int numbytes;
                if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
                    perror("recv");
                    exit(1);
                }
                buf[numbytes] = '\0';

                // create ack information
                struct message sendMsg;
                sendMsg.type = processIncomingMsg(buf, sendMsg.data);
                strcpy((char*)sendMsg.source, "SERVER");
                sendMsg.size = strlen((char*) sendMsg.data);

                // send ack information
                sendMessage(new_fd, sendMsg);
                printf("[INFO] ACK back to client.\n");

                // if drop connection actively
                if(sendMsg.type == 3 || sendMsg.type == 14){
                    isCloseConn = true;
                }
                if(sendMsg.type == 3){
                    printf("Authentication Faliure or Client already logged in. Refuse connection.\n");
                }else if(sendMsg.type == 14){
                    printf("Client Logout. Close connection.\n");
                }
            }
            printf("[INFO] Connection Closed.\n");
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }

    return 0;
}

int processIncomingMsg(char* incomingMsg, unsigned char* ackInfo){
    struct message decodedMsg = readMessage(incomingMsg);

    if(decodedMsg.type == 1){
        // seperate username and password
        unsigned char userName[MAXDATASIZE];
        unsigned char userPW[MAXDATASIZE];

        char* colon;
        colon = strchr ((char*) decodedMsg.data, ':');
        *colon = '\0';
        colon += sizeof(unsigned char);
        strcpy((char*) userName, (char*) decodedMsg.data);
        strcpy((char*) userPW, (char*) colon);

        // start login process
        bool isLoginSuccessful = attemptLogin(userName, userPW, ackInfo);
        // return ack type
        if(isLoginSuccessful){
            return 2;
        }else{
            return 3;
        }
    }else if(decodedMsg.type == 4){
        Logout(decodedMsg.source);
        return 14;
    }else if(decodedMsg.type == 12){
        printUserList();
        strcpy((char*) ackInfo, (char*) "LIST RETURN");
        return 13;
    }else if(decodedMsg.type == 9){
        
    }

    return -1;
}
