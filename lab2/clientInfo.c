#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/types.h>

#include "clientInfo.h"
#include "string.h"

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

bool attemptLogin(unsigned char* clientID, unsigned char* clientPW, unsigned char* returnMessage){
    printUserList();
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
