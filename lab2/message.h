#ifndef MES_H
#define MES_H

#define MAX_NAME 1000
#define MAX_DATA 1000

struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

struct connection {
    bool isConnected;
    bool isInSession;
    unsigned char source[MAX_NAME];
    unsigned char destAddr[MAX_NAME];
    unsigned char destPort[MAX_NAME];
    unsigned char portNum[10];
};

void sendMessage(int s, struct message encodedMsg);
struct message readMessage(char* incomingMsg);

// message types
/*
    1: [client -> server] login
        <clientID, clientPW>
    2: [server -> client] acknowledge successful login
    3: [server -> client] negative acknowledgement successful login
        <failureMsg>
    4: [client -> server] exit from the server
    5: [client -> server] join a session
        <sessionID>
    6: [server -> client] acknowledge successful join
        <sessionID>
    7: [server -> client] acknowledge unsuccessful join
        <sessionID, failureMsg>
    8: [client -> server] leave session
    9: [client -> server] create new session and join
        <sessionID>
    10:  [server -> client] acknowledge new session
    11:  [client -> server] send message in joined session
        <msg>
    12:  [client -> server] query for list of online users and avaliable sessions
    13:  [server -> client] reply query in message type 12
        <users and sessions list>
    -- extra type
    14: [server -> client] ACK for logout
    15: [server -> client] acknowledge unsuccessful created new session
    16: [server -> client] leave session successful
    17: [server -> client] leave session failed
    18: [server -> client] message sent successful
    19: [server -> client] message sent failed

*/
#endif
