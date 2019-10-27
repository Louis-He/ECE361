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

bool joinSession(unsigned char* clientID, bool newSession, unsigned char* sessionID, unsigned char* returnMessage){
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


    return true;
}

bool createSession(unsigned char* clientID, unsigned char* sessionID, unsigned char* returnMessage){
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
    joinSession(clientID, true, sessionID, returnMessage);

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
