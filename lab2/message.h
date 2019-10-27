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
    unsigned char* destAddr;
    unsigned char* destPort;
};

// message types
/*
    1: login



*/
