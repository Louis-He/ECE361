#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "message.h"
#define MAXDATASIZE 1000

void sendMessage(int s, struct message encodedMsg){
    char txt[MAXDATASIZE];
    const int len = sprintf(txt, "%d,%d,%s,%s",
        encodedMsg.type, encodedMsg.size, encodedMsg.source, encodedMsg.data);
    if(send(s, txt, strlen(txt), 0) == -1){
        perror("send");
    }
}

struct message readMessage(char* incomingMsg){
    struct message decodedMsg;
    unsigned char msgSourceAndData[MAXDATASIZE];

    sscanf(incomingMsg, "%d,%d,%[^\n]s", &decodedMsg.type, &decodedMsg.size, msgSourceAndData);
    // split soruce and data
    char* comma;
    comma = strchr ((char*) msgSourceAndData, ',');
    *comma = '\0';
    comma += sizeof(unsigned char);
    strcpy((char*) decodedMsg.source, (char*) msgSourceAndData);

    // unsigned char* msgPtr = decodedMsg.data;
    // for(int i = 0; i < decodedMsg.size; i++){
    //     printf("%c", *comma);
    //     *msgPtr = *comma;
    //     msgPtr++;comma++;
    // }
    strcpy((char*) decodedMsg.data, (char*) comma);
    // *msgPtr = '\0';
    // processing over

    return decodedMsg;
}
