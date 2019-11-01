#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#include <sys/mman.h>
#include <sys/types.h>

#include "clientInfo.h"
#include "string.h"
#include "message.h"

static struct clientInfo* currentClientInfo;
static struct sessionInfo* currentSessionInfo;

void initializeRecord(){
    // init clients
    currentClientInfo = (struct clientInfo*)mmap(NULL, sizeof(struct clientInfo) * MAX_USER, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    currentSessionInfo = (struct sessionInfo*)mmap(NULL, sizeof(struct sessionInfo) * MAX_CONN, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    for(int i = 0; i < MAX_USER; i++){
        currentClientInfo[i].isConnected = false;
        currentClientInfo[i].isInsession = false;
    }
    strcpy((char*) currentClientInfo[0].clientID, "LL");
    strcpy((char*) currentClientInfo[0].clientPW, "NB");
    strcpy((char*) currentClientInfo[1].clientID, "Louis");
    strcpy((char*) currentClientInfo[1].clientPW, "NB");

    // init sessions
    for(int i = 0; i < MAX_CONN; i++){
        currentSessionInfo[i].membernum = 0;
    }
}

bool attemptLogin(struct sockaddr socketID, unsigned char* clientID, unsigned char* clientPW, unsigned char* returnMessage, unsigned char* source){
    for(int i = 0; i < MAX_USER; i++){
        // if userID exist
        if(strcmp((char*) clientID, (char*) currentClientInfo[i].clientID) == 0){
            if(currentClientInfo[i].isConnected){
                strcpy((char*) returnMessage, "[ERROR] User already logged in.");
                return false;
            }else{
                // check the password
                if(strcmp((char*) clientPW, (char*) currentClientInfo[i].clientPW) == 0){
                    currentClientInfo[i].isConnected = true;
                    strcpy((char*) returnMessage, "[INFO] Successfully logged in.");

                    int clientIdx = findClient(clientID);
                    currentClientInfo[clientIdx].socketID = socketID;

                    struct sockaddr_in *addr_in = (struct sockaddr_in *)&socketID;
                    char *s = inet_ntoa(addr_in->sin_addr);
                    int portNum = addr_in->sin_port;
                    currentClientInfo[clientIdx].portNum = portNum;

                    char portnum[10];
                    sprintf(portnum, "%d", portNum);
                    printf("%s:%s\n", s, portnum);
                    strcpy((char*)currentClientInfo[clientIdx].ipAdd, (char*) s);

                    strcpy((char*) source, portnum);

                    return true;
                }else{
                    currentClientInfo[i].isConnected = false;
                    strcpy((char*) returnMessage, "[ERROR] Wrong Client ID or Wrong Password.");
                    return false;
                }
            }
        }
    }

    strcpy((char*) returnMessage, "[ERROR] User not found.");
    return false;
}

void printUserList(){
    printf("Active Users: \n");
    for(int i = 0; i < MAX_USER; i++){
        if(currentClientInfo[i].isConnected){
            printf("%s\n", currentClientInfo[i].clientID);
        }
    }
    return;
}

void Logout(unsigned char* clientID){
    for(int i = 0; i < MAX_USER; i++){
        if(strcmp((char*) clientID, (char*) currentClientInfo[i].clientID) == 0){
            currentClientInfo[i].isConnected = false;

            printf("[INFO] Logged out user: %s\n", currentClientInfo[i].clientID);
            return;
        }
    }
}

int findClient(unsigned char* clientID){
    for(int i = 0; i < MAX_USER; i++){
        if(strcmp((char*) currentClientInfo[i].clientID, (char*) clientID) == 0){
            return i;
        }
    }
    return -1;
}

int findSession(unsigned char* sessionID){
    for(int i = 0; i < MAX_CONN; i++){
        if(strcmp((char*) currentSessionInfo[i].sessionID, (char*) sessionID) == 0){
            return i;
        }
    }
    return -1;
}

bool clientInSession(unsigned char* clientID){
    for(int i = 0; i < MAX_USER; i++){
        if(strcmp((char*) clientID, (char*) currentClientInfo[i].clientID) == 0){
            return currentClientInfo[i].isInsession;
        }
    }
    return false;
}

bool joinSession(unsigned char* clientID, bool newSession, unsigned char* sessionID, unsigned char* returnMessage, unsigned char* source){
    int sessionIdx = -1;

    if(!newSession){
        // check if user already in a session
        if(clientInSession(clientID)){
            strcpy((char *) returnMessage, (char *)"[ERROR] Already in session.");
            return false;
        }
        // check if sessionID exists
        for(int i = 0; i < MAX_CONN; i++){
            if((strcmp((char*) currentSessionInfo[i].sessionID, (char*) sessionID) == 0) &&
        (currentSessionInfo[i].membernum != 0)){
                sessionIdx = i;
            }
        }
    }else{
        // check sessionID is enough. newSession means no member.
        for(int i = 0; i < MAX_CONN; i++){
            if((strcmp((char*) currentSessionInfo[i].sessionID, (char*) sessionID) == 0)){
                sessionIdx = i;
            }
        }
    }

    if(sessionIdx == -1){
        strcpy((char *) returnMessage, (char *)"[ERROR] Session Not Found.");
        return false;
    }

    // update session status
    currentSessionInfo[sessionIdx].membernum++;
    // update client status
    int clientIdx = findClient(clientID);
    currentClientInfo[clientIdx].isInsession = true;
    strcpy((char*) currentClientInfo[clientIdx].sessionID, (char*) sessionID);
    if(newSession){
        strcpy((char *) returnMessage, (char *)"[INFO] Successfully Created and Joined New Session.");
    }else{
        strcpy((char *) returnMessage, (char *)"[INFO] Successfully Joined Session.");
    }

    // struct sockaddr_in *addr_in = (struct sockaddr_in *)&currentClientInfo[clientIdx].socketID;
    // char *s = inet_ntoa(addr_in->sin_addr);
    // int portNum = addr_in->sin_port;
    // char portnum[10];
    // sprintf(portnum, "%d", portNum);
    // printf("%s:%s\n", s, portnum);

    return true;
}

bool createSession(unsigned char* clientID, unsigned char* sessionID, unsigned char* returnMessage, unsigned char* source){
    int avaliableSessionIdx = -1;

    // check if user already in a session
    if(clientInSession(clientID)){
        strcpy((char *) returnMessage, (char *)"[ERROR] Already in session.");
        return false;
    }
    // check if already exist sessionID
    for(int i = 0; i < MAX_CONN; i++){
        if(strcmp((char*) currentSessionInfo[i].sessionID, (char*) sessionID) == 0 &&
        currentSessionInfo[i].membernum != 0){
            strcpy((char *) returnMessage, (char *)"[ERROR] SessionID already existed.");
            return false;
        }
        if(currentSessionInfo[i].membernum == 0){
            avaliableSessionIdx = i;
        }
    }
    // check if no avaliable session slot
    if(avaliableSessionIdx == -1){
        strcpy((char *) returnMessage, (char *)"[ERROR] Maximum session achieved. No more new session.");
        return false;
    }

    // no error, create and join a new session
    newSession(sessionID, avaliableSessionIdx);
    strcpy((char *) returnMessage, (char *)"[INFO] Session created successfully.");
    joinSession(clientID, true, sessionID, returnMessage, source);

    return true;
}

void newSession(unsigned char* sessionID, int idx) {
    strcpy((char*) currentSessionInfo[idx].sessionID, (char*) sessionID);
}

bool leaveSession(unsigned char* clientID, unsigned char* returnMessage){
    int clientIdx = findClient(clientID);
    int sessionIdx = findSession(currentClientInfo[clientIdx].sessionID);

    if(!currentClientInfo[clientIdx].isInsession){
        strcpy((char *) returnMessage, (char *)"[ERROR] User not in the session.");
        return false;
    }
    if(currentSessionInfo[sessionIdx].membernum == 0){
        strcpy((char *) returnMessage, (char *)"[ERROR] Session not exist.");
        return false;
    }

    currentSessionInfo[sessionIdx].membernum --;
    currentClientInfo[clientIdx].isInsession = false;
    strcpy((char *) returnMessage, (char *)"[INFO] Successfully leave the session.");
    return true;
}

void printSessionList(){
    printf("Active Sessions: \n");
    for(int i = 0; i < MAX_CONN; i++){
        if(currentSessionInfo[i].membernum != 0){
            printf("%s\n", currentClientInfo[i].sessionID);
        }
    }
    return;
}

bool isMessageSent(unsigned char* clientID, unsigned char* Message, unsigned char* returnMessage){
    int clientIdx = findClient(clientID);
    // int sessionIdx = findSession(currentClientInfo[clientIdx].sessionID);

    // create send msg information
    struct message sendMsg;
    unsigned char messageinfo[MAX_DATA];
    sprintf((char*)messageinfo, "[%s -> %s] %s", (char*) clientID, (char*) currentClientInfo[clientIdx].sessionID, (char*) Message);

    strcpy((char*) sendMsg.data, (char*) messageinfo);
    sendMsg.type = 11;
    strcpy((char*)sendMsg.source, "SERVER");
    sendMsg.size = strlen((char*) sendMsg.data);

    for(int i = 0; i < MAX_USER; i++){
        if(currentClientInfo[i].isInsession &&
            strcmp((char*)currentClientInfo[i].sessionID, (char*)currentClientInfo[clientIdx].sessionID) == 0){

            // struct sockaddr_in *addr_in = (struct sockaddr_in *)&currentClientInfo[clientIdx].socketID;
            // char *s = inet_ntoa(addr_in->sin_addr);
            // int portNum = addr_in->sin_port + 1;
            // char portnum[10];
            // sprintf(portnum, "%d", portNum);

            // send message information
            // start new connection here
            struct addrinfo hints;
            struct addrinfo* res;
            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_INET;  // use IPv4
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

            printf("[INFO] Port: %d\n", currentClientInfo[i].portNum + 1);
            char s[200];
            strcpy(s, (char*)currentClientInfo[i].ipAdd);
            char portnum[10];
            sprintf(portnum, "%d", currentClientInfo[i].portNum + 1);

            int rv = getaddrinfo(s, portnum, &hints, &res);
            if(rv != 0){
                printf("[ERROR] Invalid IP Address or Port Number.\n");
                continue;
            }
            int tmpsocket = socket(AF_INET, SOCK_STREAM, 0);
            if(connect(tmpsocket, res->ai_addr, res->ai_addrlen) == -1){
                perror("[ERROR] Cannot Connect To the client");
                continue;
            }
            printf("connected to client\n");
            printf("[INFO] Message to client: %s\n", currentClientInfo[i].clientID);

            sendMessage(tmpsocket, sendMsg);

            close(tmpsocket);
        }
    }

    strcpy((char*)returnMessage, "[INFO] Sending message over.");
    return true;
}
