#define _POSIX_C_SOURCE 200112L
#define MAXDATASIZE 1000

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>

#include <sys/time.h>

#include "clientInfo.h"
#include "server.h"
#include "message.h"

#define BACKLOG 10   // how many pending connections queue will hold


/*  * Part of the code is cited from https://beej.us/guide/bgnet/  */
void *myThreadFun(void *vargp);

int main(int argc, char** argv) {
    if(argc != 2){
        printf("[Error] Not enough or too many arguments.\n");
        return (EXIT_FAILURE);
    }

    // read command
    pthread_t tid;
    pthread_create(&tid, NULL, myThreadFun, (void *)&tid);

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
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    bind(s, res->ai_addr, res->ai_addrlen);

    if(listen(s, BACKLOG) == -1){
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    // start transmission
    struct sockaddr their_addr; // connector's address information
    socklen_t sin_size;

    char buf[MAXDATASIZE];
    while (true) {
        int new_fd;
        sin_size = sizeof their_addr;
        new_fd = accept(s, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        printf("%d\n", new_fd);

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
                memset(sendMsg.data, 0, sizeof MAX_DATA);
                sendMsg.type = processIncomingMsg(their_addr, buf, sendMsg.data, sendMsg.source);
                sendMsg.size = strlen((char*) sendMsg.data);

                // send ack information
                if(sendMsg.type != 18){
                    sendMessage(new_fd, sendMsg);
                    printf("[INFO] ACK back to client.\n");
                }

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

    // stop reding command
    pthread_cancel(tid);

    return 0;
}

int processIncomingMsg(struct sockaddr socketID, char* incomingMsg, unsigned char* ackInfo, unsigned char* source){
    struct message decodedMsg = readMessage(incomingMsg);
    strcpy((char*)source, "SERVER");

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
        bool isLoginSuccessful = attemptLogin(socketID, userName, userPW, ackInfo, source);
        // return ack type
        if(isLoginSuccessful){
            return 2;
        }else{
            return 3;
        }
    }else if(decodedMsg.type == 4){
        if(clientInSession(decodedMsg.source)){
            bool isLeaveSuccess = leaveSession(decodedMsg.source, ackInfo);
        }
        Logout(decodedMsg.source);
        return 14;
    }else if(decodedMsg.type == 12){
        getActiveUserList(ackInfo);
        getAvaliableSession(ackInfo);
        printf("%s\n", ackInfo);
        return 13;
    }else if(decodedMsg.type == 9){
        bool isCreateSuccess = createSession(decodedMsg.source, decodedMsg.data, ackInfo, source);
        if(isCreateSuccess){
            return 10;
        }else{
            return 15;
        }
    }else if(decodedMsg.type == 8){
        bool isLeaveSuccess = leaveSession(decodedMsg.source, ackInfo);
        if(isLeaveSuccess){
            return 16;
        }else{
            return 17;
        }
    }else if(decodedMsg.type == 5){
        bool isJoinSuccess = joinSession(decodedMsg.source, false, decodedMsg.data, ackInfo, source);

        unsigned char broadCastMsgReturn[MAX_DATA];
        unsigned char broadCastMsg[MAX_DATA];

        strcpy((char*) broadCastMsg, (char*) "[System INFO] ");
        strcat((char*) broadCastMsg, (char*) decodedMsg.source);
        strcat((char*) broadCastMsg, (char*)  " joined the session.");

        isMessageSent(decodedMsg.source, (unsigned char*)broadCastMsg, broadCastMsgReturn);

        if(isJoinSuccess){
            return 6;
        }else{
            return 7;
        }
    }else if(decodedMsg.type == 11){
        isMessageSent(decodedMsg.source, decodedMsg.data, ackInfo);
        return 18;
    }else if(decodedMsg.type == 20){
        bool isInviteSuccess = inviteUser(decodedMsg.source, decodedMsg.data, ackInfo);

        if(isInviteSuccess){
            return 21;
        }else{
            return 22;
        }
    }else if(decodedMsg.type == 24){
        bool isJoinSuccess = responseInvitationUser(decodedMsg.source, true, ackInfo);
        if(isJoinSuccess){
            return 26;
        }else{
            return 27;
        }
    }else if(decodedMsg.type == 25){
        bool isDeclineSuccess = responseInvitationUser(decodedMsg.source, false, ackInfo);
        return 26;
    }else if(decodedMsg.type == 29){
        // seperate destClient and message
        unsigned char destClient[MAXDATASIZE];
        unsigned char message[MAXDATASIZE];

        char* colon;
        colon = strchr ((char*) decodedMsg.data, ':');
        *colon = '\0';
        colon += sizeof(unsigned char);
        strcpy((char*) destClient, (char*) decodedMsg.data);
        strcpy((char*) message, (char*) colon);
        
        bool isSentWhisper = whisperClient(decodedMsg.source, destClient, message, ackInfo);

        printf("[TEST]%s\n", ackInfo);
        if(isSentWhisper){
            return 30;
        }else {
            return 31;
        }
    }

    return -1;
}

void *myThreadFun(void *vargp){
    char command[100];

    while(true){
        scanf("%s", command);

        // TODO
        unsigned char returnMessage[MAXDATASIZE];
        broadCastMessageSent((unsigned char*)command, returnMessage);
    }

    return NULL;
}
