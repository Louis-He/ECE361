#ifndef CLIENTINFO_H
#define CLIENTINFO_H

#include <sys/types.h>
#include <sys/socket.h>

#define MAX_ATTR_LEN 200
#define MAX_CONN 10
#define MAX_USER 10

struct clientInfo {
    unsigned char clientID[MAX_ATTR_LEN];
    unsigned char clientPW[MAX_ATTR_LEN];
    unsigned char sessionID[MAX_ATTR_LEN];
    struct sockaddr clientIPandPort;

    bool isConnected;
    bool isInsession;
};

struct sessionInfo {
    int membernum;
    unsigned char sessionID[MAX_ATTR_LEN];
};

void initializeRecord();
bool attemptLogin(unsigned char* clientID, unsigned char* clientPW,
    unsigned char* returnMessage);
void printUserList();
void Logout(unsigned char* clientID);

#endif
