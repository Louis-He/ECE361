// #define _POSIX_C_SOURCE 200112L
#define MAXDATASIZE 1000
#define BACKLOG 10

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>

#include <sys/time.h>
#include "client.h"
#include "message.h"

/*  * Part of the code is cited from https://beej.us/guide/bgnet/  */
// socket()
struct connection connectionInfo;
int* isInvitedBool;
pthread_t tid;
int s2;

int main(int argc, char** argv){
    isInvitedBool = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int s;
    printf("[Info] Text Conference Service Start. Please connect to a server.\n");

    connectionInfo.isConnected = false;
    connectionInfo.isInSession = false;
    *isInvitedBool = 0;
    // Start connection here

    char buf[MAXDATASIZE];

    // connection
    struct addrinfo hints;
    struct addrinfo* res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;  // use IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    // initialize some data
    unsigned char* commandIn[5];
    unsigned char* encodedData;
    for(int i = 0; i < 5; i++){
        commandIn[i] = (unsigned char*)malloc(sizeof(unsigned char) * MAX_COMMAND_LEN);
    }
    // initialize the size of encodedData, it will change each time in the loop
    encodedData = (unsigned char*)malloc(sizeof(unsigned char) * MAX_COMMAND_LEN);
    // loop for getting input from user
    while(true){
        for(int i = 0; i < 5; i++){
            memset (commandIn[i], '\0', sizeof(unsigned char) * MAX_COMMAND_LEN);
        }
        memset (encodedData, '\0', sizeof(unsigned char) * MAX_COMMAND_LEN);
        int isLoop = readInAndProcessCommand(commandIn, encodedData);

        if(isLoop == 1){
            // valid input
            struct message sendMsg;

            if(strcmp((char*)commandIn[0], "/login") == 0){
                if(connectionInfo.isConnected){
                    printf("[ERROR] Already connected. Please drop the connection first.\n");
                }else{
                    bool isConnected = true;
                    // start new connection here
                    int rv = getaddrinfo((char *)commandIn[3], (char *)commandIn[4], &hints, &res);
                    if(rv != 0){
                        printf("[ERROR] Invalid IP Address or Port Number.\n");
                        continue;
                    }
                    s = socket(AF_INET, SOCK_STREAM, 0);
                    if(connect(s, res->ai_addr, res->ai_addrlen) == -1){
                        isConnected = false;
                        perror("[ERROR] Cannot Connect To the Server");
                        continue;
                    }
                    printf("[INFO] Try to Login\n");

                    // create login info pack
                    sendMsg.type = 1;
                    sendMsg.size = strlen((char*) encodedData);
                    strcpy((char*) sendMsg.source, (char*) commandIn[1]);
                    strcpy((char*) sendMsg.data, (char*) encodedData);
                    // send login info
                    // printf("[INFO] Message ready\n");
                    sendMessage(s, sendMsg);

                    // received from server to comfirm connection
                    int numbytes = recv(s, buf, MAXDATASIZE-1, 0);
                    if (numbytes == -1) {
                        perror("recv");
                        exit(1);
                    }
                    buf[numbytes] = '\0';

                    struct message decodedMsg = readMessage(buf);
                    // printf("%d %d %s %s\n", decodedMsg.type, decodedMsg.size,
                    // decodedMsg.source, decodedMsg.data);

                    if(decodedMsg.type == 2){
                        // store client id and connection status
                        connectionInfo.isConnected = true;
                        strcpy((char*) connectionInfo.portNum, (char*) decodedMsg.source);
                        strcpy((char*) connectionInfo.source, (char*) sendMsg.source);
                        strcpy((char*) connectionInfo.destAddr, (char*) commandIn[3]);
                        strcpy((char*) connectionInfo.destPort, (char*) commandIn[4]);
                        printf("%s\n", decodedMsg.data);

                        // open listening sorkect
                        // create listening thread
                        pthread_create(&tid, NULL, myThreadFun, (void *)&tid);
                    }else{
                        connectionInfo.isConnected = false;
                        printf("%s\n", decodedMsg.data);
                        // drop connection if login unsuccessful
                        if(isConnected){
                            printf("[INFO] Connection Closed\n");
                            close(s);
                        }
                    }
                }
            }else{
                if(!connectionInfo.isConnected){
                    printf("[ERROR] Not connected to any server.\n");
                    continue;
                }

                if(strcmp((char*)commandIn[0], "/list") == 0){
                    // create logout info pack
                    sendMsg.type = 12;
                    sendMsg.size = strlen((char*) "list");
                    strcpy((char*) sendMsg.source,(char*) connectionInfo.source);
                    strcpy((char*) sendMsg.data, "list");
                    // send logout info
                    sendMessage(s, sendMsg);

                    // received from server to comfirm FIN
                    int numbytes = recv(s, buf, MAXDATASIZE-1, 0);
                    if (numbytes == -1) {
                        perror("recv");
                        exit(1);
                    }
                    buf[numbytes] = '\0';

                    struct message decodedMsg = readMessage(buf);
                    printf("%s\n", decodedMsg.data);

                }else if(strcmp((char*)commandIn[0], "/createsession") == 0){
                    if(connectionInfo.isInSession){
                        printf("[ERROR] Already in session.\n");
                        continue;
                    }
                    // create create session info pack
                    sendMsg.type = 9;
                    sendMsg.size = strlen((char*) commandIn[1]);
                    strcpy((char*) sendMsg.source, (char*) connectionInfo.source);
                    strcpy((char*) sendMsg.data, (char*) commandIn[1]);
                    // send create session info
                    // printf("[INFO] Message ready\n");
                    sendMessage(s, sendMsg);

                    // received from server to comfirm successfully create and joinsession
                    int numbytes = recv(s, buf, MAXDATASIZE-1, 0);
                    if (numbytes == -1) {
                        perror("recv");
                        exit(1);
                    }
                    buf[numbytes] = '\0';

                    struct message decodedMsg = readMessage(buf);
                    if(decodedMsg.type == 10){
                        printf("%s\n", decodedMsg.data);
                        // create session successfully
                        connectionInfo.isInSession = true;
                    }else if(decodedMsg.type == 15){
                        printf("%s\n", decodedMsg.data);
                    }
                }else if(strcmp((char*)commandIn[0], "/joinsession") == 0){
                    if(connectionInfo.isInSession){
                        printf("[ERROR] Already in session.\n");
                        continue;
                    }

                    // create join session info pack
                    sendMsg.type = 5;
                    sendMsg.size = strlen((char*) commandIn[1]);
                    strcpy((char*) sendMsg.source, (char*) connectionInfo.source);
                    strcpy((char*) sendMsg.data, (char*) commandIn[1]);
                    // send create session info
                    sendMessage(s, sendMsg);

                    // received from server to comfirm successfully create and joinsession
                    int numbytes = recv(s, buf, MAXDATASIZE-1, 0);

                    if (numbytes == -1) {
                        perror("recv");
                        exit(1);
                    }
                    buf[numbytes] = '\0';
                    // printf("%s\n", buf);

                    struct message decodedMsg = readMessage(buf);
                    if(decodedMsg.type == 6){
                        printf("%s\n", decodedMsg.data);
                        // join session successfully
                        connectionInfo.isInSession = true;

                        // // create listening thread
                        // pthread_t tid;
                        // pthread_create(&tid, NULL, myThreadFun, (void *)&tid);
                    }else if(decodedMsg.type == 7){
                        printf("%s\n", decodedMsg.data);
                    }
                }else if(strcmp((char*)commandIn[0], "/leavesession") == 0){
                    if(!connectionInfo.isInSession){
                        printf("[ERROR] Not in any session. Join a session first.\n");
                        continue;
                    }
                    // create leave session info pack
                    sendMsg.type = 8;
                    sendMsg.size = 0;
                    strcpy((char*) sendMsg.source, (char*) connectionInfo.source);
                    strcpy((char*) sendMsg.data, "");
                    // send leave session info
                    // printf("[INFO] Message ready\n");
                    sendMessage(s, sendMsg);

                    // received from server to comfirm successfully create and joinsession
                    int numbytes = recv(s, buf, MAXDATASIZE-1, 0);
                    if (numbytes == -1) {
                        perror("recv");
                        exit(1);
                    }
                    buf[numbytes] = '\0';

                    struct message decodedMsg = readMessage(buf);

                    if(decodedMsg.type == 16){
                        // close(s2);
                        //pthread_cancel(tid);
                        connectionInfo.isInSession = false;
                    }
                    printf("%s\n", decodedMsg.data);

                }else if(strcmp((char*)commandIn[0], "/logout") == 0){
                    // create logout info pack
                    sendMsg.type = 4;
                    sendMsg.size = strlen((char*) "EXIT");
                    strcpy((char*) sendMsg.source,(char*) connectionInfo.source);
                    strcpy((char*) sendMsg.data, "EXIT");
                    // send logout info
                    sendMessage(s, sendMsg);

                    // received from server to comfirm FIN
                    int numbytes = recv(s, buf, MAXDATASIZE-1, 0);
                    if (numbytes == -1) {
                        perror("recv");
                        exit(1);
                    }
                    buf[numbytes] = '\0';

                    struct message decodedMsg = readMessage(buf);
                    if(decodedMsg.type == 14){
                        // change connection status
                        connectionInfo.isConnected = false;
                        connectionInfo.isInSession = false;

                        // close listening thread
                        close(s2);
                        pthread_cancel(tid);
                        printf("[INFO] Connection Closed\n");
                    }
                }else if(strcmp((char*)commandIn[0], "/invite") == 0){
                    if(!connectionInfo.isInSession){
                        printf("[ERROR] Not in any session. Join a session first.\n");
                        continue;
                    }

                    // create leave session info pack
                    sendMsg.type = 20;
                    strcpy((char*) sendMsg.source, (char*) connectionInfo.source);
                    strcpy((char*) sendMsg.data, (char*) commandIn[1]);
                    sendMsg.size = strlen((char*) commandIn[1]);
                    // send message
                    sendMessage(s, sendMsg);

                    // received from server to comfirm successfully create and joinsession
                    int numbytes = recv(s, buf, MAXDATASIZE-1, 0);

                    if (numbytes == -1) {
                        perror("recv");
                        exit(1);
                    }
                    buf[numbytes] = '\0';

                    struct message decodedMsg = readMessage(buf);
                    printf("%s\n", decodedMsg.data);
                }else if(strcmp((char*)commandIn[0], "/accept") == 0){
                    if(*isInvitedBool == 0){
                        printf("[ERROR] Not have any invitation.\n");
                        continue;
                    }
                    if(connectionInfo.isInSession){
                        printf("[ERROR] Already in session. Please leave session first.\n");
                        continue;
                    }
                    // create accept session info pack
                    sendMsg.type = 24;
                    strcpy((char*) sendMsg.source, (char*) connectionInfo.source);
                    strcpy((char*) sendMsg.data, (char*) "ACCEPT");
                    sendMsg.size = strlen((char*) "ACCEPT");
                    // send message
                    sendMessage(s, sendMsg);

                    // received from server to comfirm successfully create and joinsession
                    int numbytes = recv(s, buf, MAXDATASIZE-1, 0);

                    if (numbytes == -1) {
                        perror("recv");
                        exit(1);
                    }
                    buf[numbytes] = '\0';

                    struct message decodedMsg = readMessage(buf);
                    printf("%s\n", decodedMsg.data);

                    if(decodedMsg.type == 26){
                        connectionInfo.isInSession = true;
                    }
                    *isInvitedBool = 0;
                }else if(strcmp((char*)commandIn[0], "/decline") == 0){
                    if(*isInvitedBool == 0){
                        printf("[ERROR] Not have any invitation.\n");
                        continue;
                    }

                    // create decline session info pack
                    sendMsg.type = 25;
                    strcpy((char*) sendMsg.source, (char*) connectionInfo.source);
                    strcpy((char*) sendMsg.data, (char*) "DECLINE");
                    sendMsg.size = strlen((char*) "DECLINE");
                    // send message
                    sendMessage(s, sendMsg);

                    // received from server to comfirm successfully create and joinsession
                    int numbytes = recv(s, buf, MAXDATASIZE-1, 0);

                    if (numbytes == -1) {
                        perror("recv");
                        exit(1);
                    }
                    buf[numbytes] = '\0';

                    struct message decodedMsg = readMessage(buf);
                    printf("%s\n", decodedMsg.data);

                    *isInvitedBool = 0;
                }
            }

        }else if (isLoop == 0){
            printf("[INFO] Quit Text Conference Service.\n");
            printf("************************************\n");
            printf("      Have a good day. Bye.         \n");
            break;
        }else if (isLoop == 2){
            printf("[ERROR] Invalid Command, Please Check.\n");
        }else if (isLoop == 3){
            if(!connectionInfo.isConnected){
                printf("[ERROR] Not connected to any server.\n");
                continue;
            }
            if(!connectionInfo.isInSession){
                printf("[ERROR] Not joined any session.\n");
                continue;
            }

            // create message info pack
            struct message sendMsg;

            sendMsg.type = 11;
            sendMsg.size = strlen((char*) commandIn[1]);
            strcpy((char*) sendMsg.source, (char*) connectionInfo.source);
            strcpy((char*) sendMsg.data, (char*) commandIn[1]);
            // send create session info
            // printf("[INFO] Message ready\n");
            sendMessage(s, sendMsg);

            // // received from server to comfirm successfully create and joinsession
            // int numbytes = recv(s, buf, MAXDATASIZE-1, 0);
            // if (numbytes == -1) {
            //     perror("recv");
            //     exit(1);
            // }
            // buf[numbytes] = '\0';
            //
            // struct message decodedMsg = readMessage(buf);
            //
            // printf("Message\n");
        }else{
            printf("[ERROR] Unknown Error.\n");
        }
    }

    return 0;
}

void *myThreadFun(void *vargp){
    // socket()
    struct addrinfo hints;
    struct addrinfo* res;
    int yes=1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    int originPort;
    sscanf((char*)connectionInfo.portNum, "%d", &originPort);
    char newPort[10];
    sprintf(newPort, "%d", originPort + 1);
    int rv = getaddrinfo(NULL, newPort, &hints, &res);

    s2 = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    setsockopt(s2, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    bind(s2, res->ai_addr, res->ai_addrlen);

    if(listen(s2, BACKLOG) == -1){
        perror("listen");
        exit(1);
    }

    printf("Client Port #%s: waiting for message...\n", newPort);
    struct sockaddr their_addr; // connector's address information
    socklen_t sin_size;
    while(true){
        int new_fd;
        sin_size = sizeof their_addr;
        new_fd = accept(s2, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            pthread_exit(0);
        }

        if (!fork()) { // this is the child process
            close(s2); // child doesn't need the listener
            char buf[MAXDATASIZE];
            // received from server to comfirm connection
            // int numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0);
            int numbytes;
            if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
                perror("recv");
                exit(1);
            }
            buf[numbytes] = '\0';

            struct message decodedMsg = readMessage(buf);

            if(decodedMsg.type == 11){
                printf("%s\n", decodedMsg.data);
            }else if(decodedMsg.type == 23){
                printf("%s\n", decodedMsg.data);
                *isInvitedBool = 1;
            }

            close(new_fd);
            exit(0);
        }
    }
    return NULL;
}

int readInAndProcessCommand(unsigned char* commandLine[5], unsigned char* encodedData){
    unsigned char incomingMsg[MAXDATASIZE];
    memset (incomingMsg, '\0', sizeof(unsigned char) * MAXDATASIZE);
    scanf("%[^\n]s", incomingMsg);
    setbuf(stdin, NULL);

    sscanf((char*) incomingMsg, "%s", (char*)commandLine[0]);

    if(strcmp((char*)commandLine[0], "/login") == 0){
        sscanf((char*) incomingMsg, "%s %s %s %s %s", (char*) commandLine[0],
            (char*) commandLine[1],
            (char*) commandLine[2],
            (char*) commandLine[3],
            (char*) commandLine[4]);
        unsignedStrCopy(encodedData, commandLine[1]);
        int tmpLen = strlen((char *)encodedData);
        encodedData[tmpLen] = ':';
        encodedData[tmpLen + 1] = '\0';
        unsignedStrCopy(encodedData, commandLine[2]);

        return 1;
    } else if(strcmp((char*)commandLine[0], "/logout") == 0){
        return 1;
    } else if(strcmp((char*)commandLine[0], "/joinsession") == 0){
        sscanf((char*) incomingMsg, "%s %s", (char*) commandLine[0], (char*) commandLine[1]);
        strcpy((char*)encodedData, (char*)commandLine[1]);
        return 1;
    } else if(strcmp((char*)commandLine[0], "/leavesession") == 0){
        return 1;
    } else if(strcmp((char*)commandLine[0], "/createsession") == 0){
        sscanf((char*) incomingMsg, "%s %s", (char*) commandLine[0], (char*) commandLine[1]);
        strcpy((char*)encodedData, (char*)commandLine[1]);
        return 1;
    } else if(strcmp((char*)commandLine[0], "/list") == 0){
        return 1;
    } else if(strcmp((char*)commandLine[0], "/invite") == 0){
        sscanf((char*) incomingMsg, "%s %s", (char*) commandLine[0], (char*) commandLine[1]);
        strcpy((char*)encodedData, (char*)commandLine[1]);
        return 1;
    } else if(strcmp((char*)commandLine[0], "/accept") == 0){
        return 1;
    } else if(strcmp((char*)commandLine[0], "/decline") == 0){
        return 1;
    } else if(strcmp((char*)commandLine[0], "/quit") == 0){
        return 0;
    } else if(strlen((char*)commandLine[0]) > 0 && commandLine[0][0] == '/'){
        return 2;
    }

    strcpy((char*) commandLine[1], (char*) incomingMsg);
    return 3;
}

// helper functions below
void unsignedStrCopy(unsigned char* dst, unsigned char* src){
    unsigned char* currentDst = dst;
    unsigned char* currentSrc = src;

    int countBreaker = 0;
    while(*currentDst != '\0'){
        countBreaker++;
        currentDst++;
    }

    while(*currentSrc != '\0'){
        *currentDst = *currentSrc;
        currentDst++;
        currentSrc++;
    }

    *currentDst = '\0';
}
