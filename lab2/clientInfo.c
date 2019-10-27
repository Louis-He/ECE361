#include <stdio.h>
#include <stdlib.h>

#include "clientInfo.h"
#include "string.h"

struct clientInfo currentClientInfo[MAX_USER];
struct sessionInfo currentSessionInfo[MAX_CONN];

void initializeRecord(){
    // init clients
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


bool attemptLogin(unsigned char* clientID, unsigned char* clientPW,
    unsigned char* returnMessage){
    for(int i = 0; i < MAX_USER; i++){
        if(strcmp((char*) clientID, (char*) currentClientInfo[i].clientID) == 0){
            if(currentClientInfo[i].isConnected){
                strcpy((char*) returnMessage, "[ERROR] User already logged in.");
                return false;
            }else{
                currentClientInfo[i].isConnected = true;
                strcpy((char*) returnMessage, "[INFO] Successfully logged in.");
                return true;
            }
        }
    }

    strcpy((char*) returnMessage, "[ERROR] User not found.");
    return false;
}
