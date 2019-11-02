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
    unsigned char ipAdd[MAX_ATTR_LEN];
    int portNum;
    struct sockaddr socketID;

    bool isConnected;
    bool isInsession;
};

struct sessionInfo {
    int membernum;
    unsigned char sessionID[MAX_ATTR_LEN];
};

void initializeRecord();
bool attemptLogin(struct sockaddr socketID, unsigned char* clientID, unsigned char* clientPW,
    unsigned char* returnMessage, unsigned char* source);
void printUserList();
void getActiveUserList(unsigned char* activeUserList);
void getAvaliableSession(unsigned char* activeSessionList);
void Logout(unsigned char* clientID);

int findClient(unsigned char* clientID);
int findSession(unsigned char* sessionID);

bool clientInSession(unsigned char* clientID);
bool joinSession(unsigned char* clientID, bool newSession, unsigned char* sessionID, unsigned char* returnMessage, unsigned char* source);
bool createSession(unsigned char* clientID, unsigned char* sessionID, unsigned char* returnMessage, unsigned char* source);
void newSession(unsigned char* sessionID, int idx);
bool leaveSession(unsigned char* clientID, unsigned char* returnMessage);
void printSessionList();

bool isMessageSent(unsigned char* clientID, unsigned char* Message, unsigned char* returnMessage);
bool broadCastMessageSent(unsigned char* Message, unsigned char* returnMessage);
#endif
