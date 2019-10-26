#define _POSIX_C_SOURCE 200112L
#define FRAG_SIZE 1000

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <sys/time.h>
#include "client.h"
#include "message.h"

int main(int argc, char** argv){
    printf("[Info] Text Conference Service Start. Please connect to a server.\n");

    struct connection connectionInfo;
    connectionInfo.isConnected = false;
    connectionInfo.isInSession = false;
    // Start connection here
    // // socket()
    // int s = socket(AF_INET, SOCK_DGRAM, 0);
    //
    // // connection
    // struct addrinfo hints;
    // struct addrinfo* res;
    // memset(&hints, 0, sizeof hints);
    // hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
    // hints.ai_socktype = SOCK_STREAM;
    // hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
    //
    // int rv = getaddrinfo(destAddr, portNum, &hints, &res);
    //
    // inet_ntop(hints.ai_family, get_in_addr((struct sockaddr *)res),
    //         s, sizeof s);

    // initialize some data
    unsigned char* commandIn[5];
    unsigned char* encodedData;
    for(int i = 0; i < 5; i++){
        commandIn[i] = (unsigned char*)malloc(sizeof(unsigned char) * MAX_COMMAND_LEN);
    }
    // initialize the size of encodedData, it will change each time in the loop
    encodedData = (unsigned char*)malloc(sizeof(unsigned char) * MAX_COMMAND_LEN);
    // loop for getting input from user
    while(true){
        for(int i = 0; i < 5; i++){
            memset (commandIn[i], '\0', sizeof(unsigned char) * MAX_COMMAND_LEN);
        }
        memset (encodedData, '\0', sizeof(unsigned char) * MAX_COMMAND_LEN);
        int isLoop = readInAndProcessCommand(commandIn, encodedData);
        printf("%s\n", encodedData);

        if(isLoop == 1){
            // valid input
            struct message sendMsg;
            if(strcmp((char*)commandIn[0], "login") == 0){
                if(connectionInfo.isConnected){
                    printf("[ERROR] Already connected. Please drop the connection first.\n");
                }
            }else{
                if(!connectionInfo.isConnected){
                    printf("[ERROR] Not connected to any server.\n");
                    continue;
                }

                if(strcmp((char*)commandIn[0], "logout") == 0){

                }
            }

        }else if (isLoop == 0){
            break;
        }else if (isLoop == 2){
            printf("[ERROR] Invalid Command, Please Check.\n");
        }else{
            printf("[ERROR] Unknown Error.\n");
        }
    }

    // End connection here

    return 0;
}

int readInAndProcessCommand(unsigned char* commandLine[5], unsigned char* encodedData){
    scanf("%s", commandLine[0]);

    if(strcmp((char*)commandLine[0], "login") == 0){
        scanf("%s", commandLine[1]);
        scanf("%s", commandLine[2]);
        scanf("%s", commandLine[3]);
        scanf("%s", commandLine[4]);

        encodedData = (unsigned char*)realloc(encodedData, sizeof(unsigned char) * (
            strlen((char*)commandLine[1]) + strlen((char*)commandLine[2]) + 1
        ));
        unsignedStrCopy(encodedData, commandLine[1]);
        int tmpLen = strlen((char *)encodedData);
        encodedData[tmpLen] = ':';
        encodedData[tmpLen + 1] = '\0';
        unsignedStrCopy(encodedData, commandLine[2]);

        return 1;
    } else if(strcmp((char*)commandLine[0], "logout") == 0){
        return 1;
    } else if(strcmp((char*)commandLine[0], "joinsession") == 0){
        scanf("%s", commandLine[1]);
        encodedData = (unsigned char*)realloc(encodedData, sizeof(unsigned char) * strlen((char *)commandLine[1]));
        strcpy((char*)encodedData, (char*)commandLine[1]);
        return 1;
    } else if(strcmp((char*)commandLine[0], "leavesession") == 0){
        return 1;
    } else if(strcmp((char*)commandLine[0], "createsession") == 0){
        scanf("%s", commandLine[1]);
        encodedData = (unsigned char*)realloc(encodedData, sizeof(unsigned char) * strlen((char *)commandLine[1]));
        strcpy((char*)encodedData, (char*)commandLine[1]);
        return 1;
    } else if(strcmp((char*)commandLine[0], "list") == 0){
        return 1;
    } else if(strcmp((char*)commandLine[0], "quit") == 0){
        return 0;
    }

    return 2;
}

// helper functions below
void unsignedStrCopy(unsigned char* dst, unsigned char* src){
    unsigned char* currentDst = dst;
    unsigned char* currentSrc = src;

    int countBreaker = 0;
    while(*currentDst != '\0'){
        countBreaker++;
        currentDst++;
    }

    while(*currentSrc != '\0'){
        *currentDst = *currentSrc;
        currentDst++;
        currentSrc++;
    }

    *currentDst = '\0';
}

// int dataToStr(unsigned char* commandLine[5], unsigned char* dataField, int commandArgc){
//     int totalLength = 0;
//
//     /* Find the length of data and malloc the size of the str --- START --- */
//     int currentCommand = 0;
//     while(currentCommand != commandArgc){
//         totalLength += strlen((char *)commandLine[currentCommand]);
//         currentCommand++;
//     }
//     dataField = (unsigned char*)realloc(dataField, sizeof(unsigned char) * (totalLength + commandArgc - 1));
//     for(int i = 0 ; i < totalLength + commandArgc - 1; i++){
//         dataField[i] = '\0';
//     }
//     /* Find the length of data and malloc the size of the str --- END --- */
//
//
//     // add commandData to data field
//     currentCommand = 0;
//     while(currentCommand != commandArgc){
//         unsignedStrCopy(dataField, commandLine[currentCommand]);
//         if(currentCommand != commandArgc - 1){
//             int tmpLen = strlen((char *)dataField);
//             dataField[tmpLen] = ':';
//             dataField[tmpLen + 1] = '\0';
//         }
//         currentCommand++;
//     }
//
//     return totalLength;
// }
