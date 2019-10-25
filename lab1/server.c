/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   main.c
 * Author: hesiwei1
 *
 * Created on September 12, 2019, 3:29 PM
 */

/**
 * Part of the code is cited from https://beej.us/guide/bgnet/
 */

#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "packet.h"

#define BACKLOG 10
#define BUFFER_SIZE 1100

void analyzePackets(unsigned char* buffer, struct packet* singlePacket);

/*
 *
 */
int main(int argc, char** argv) {
    if(argc != 2){
        printf("[Error] Not enough or too many arguments.\n");
        return (EXIT_FAILURE);
    }

    char* portNum = argv[1];

    // socket()
    struct addrinfo hints;
    struct addrinfo* res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    int rv = getaddrinfo(NULL, portNum, &hints, &res);

    int s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    // bind()
    bind(s, res->ai_addr, res->ai_addrlen);

    printf("listener: waiting to recvfrom...\n");

    // recvfrom()
    struct sockaddr_storage incoming_addr;
    socklen_t incoming_addr_size = sizeof incoming_addr;

    char buf[BUFFER_SIZE];
    int numbytes = recvfrom(s, buf, BUFFER_SIZE - 1, 0, (struct sockaddr *)&incoming_addr, &incoming_addr_size);
    if(numbytes == -1){
        perror("recvfrom");
    }


    buf[numbytes] = '\0';

    if(strcmp(buf, "ftp") == 0){
        printf("[INFO] Ftp service is requested.\n");
        // send info back to the client
        numbytes = sendto(s, "yes", 3, MSG_CONFIRM, (struct sockaddr *)&incoming_addr, incoming_addr_size);
    }else{
        printf("listener: packet contains \"%s\"\n", buf);
        // send info back to the client
        numbytes = sendto(s, "no", 2, MSG_CONFIRM, (struct sockaddr *)&incoming_addr, incoming_addr_size);
    }

    if (numbytes == -1){
        printf("[Error] Fail to send info back to server.\n");
        return (EXIT_FAILURE);
    }else {
        printf("[INFO] Success sending info back to server.\n");
    }

    unsigned char incommingBuf[BUFFER_SIZE];
    bool isTransferStarted = false;
    bool isTransferEnded = false;

    // streaming to file
    FILE *file;

    while(!isTransferEnded){
        numbytes = recvfrom(s, incommingBuf, BUFFER_SIZE - 1, 0, (struct sockaddr *)&incoming_addr, &incoming_addr_size);
        if(numbytes == -1){
            perror("recvfrom");
        }
        incommingBuf[numbytes] = '\0';

        // transfer buffer string to packet struct
        struct packet singlePacket;
        analyzePackets(incommingBuf, &singlePacket);
//        printf("%d: %s\n", singlePacket.frag_no, singlePacket.filedata);

        // streaming to file
        if(!isTransferStarted){
            char final[strlen(singlePacket.filename) + 8 + 1];
            final[0] = 't';
            final[1] = 'r';
            final[2] = 'a';
            final[3] = 'n';
            final[4] = 's';
            final[5] = 'f';
            final[6] = 'e';
            final[7] = 'r';
            final[8] = '/';
            for(int i = 0; i < strlen(singlePacket.filename); i++){
                final[i + 9] = singlePacket.filename[i];
            }
            printf("[INFO] %s Transfer Started.\n", final);
            file = fopen(final, "wb");
        }
        fwrite(singlePacket.filedata, 1, singlePacket.size, file);
        // printf("%s, %d\n", singlePacket.filedata, singlePacket.size);


        // send info back to the client
        numbytes = sendto(s, "yes", 3, MSG_CONFIRM, (struct sockaddr *)&incoming_addr, incoming_addr_size);
        isTransferStarted = true;
        if(singlePacket.frag_no == singlePacket.total_frag){
            isTransferEnded = true;
            printf("[INFO] Transfer Ended..\n");
        }
    }
    fclose(file);

    return (EXIT_SUCCESS);
}

void analyzePackets(unsigned char* buffer, struct packet* singlePacket){
    int firstcolon;
    int secondcolon;
    int thirdcolon;
    int forthcolon;

    char* total_frag_str;
    unsigned int total_frag;
    char* frag_no_str;
    unsigned int frag_no;
    char* size_str;
    unsigned int size;
    char* filename;
    char filedata[1000];

    // find first segment of the buffer
    for(int i = 0; i < BUFFER_SIZE; i++){
        if(buffer[i] == ':'){
            firstcolon = i;
            break;
        }
    }
    total_frag_str = (char*)malloc(sizeof(char) * firstcolon);
    for(int i = 0; i < firstcolon; i++){
        total_frag_str[i] = buffer[i];
    }
    total_frag = atoi(total_frag_str);

    // find second segment of the buffer
    for(int i = firstcolon + 1; i < BUFFER_SIZE; i++){
        if(buffer[i] == ':'){
            secondcolon = i;
            break;
        }
    }
    frag_no_str = (char*)malloc(sizeof(char) * (secondcolon - firstcolon - 1));
    for(int i = 0; i < secondcolon - firstcolon - 1; i++){
        frag_no_str[i] = buffer[firstcolon + 1 + i];
    }
    frag_no = atoi(frag_no_str);

    // find third segment of the buffer
    for(int i = secondcolon + 1; i < BUFFER_SIZE; i++){
        if(buffer[i] == ':'){
            thirdcolon = i;
            break;
        }
    }
    size_str = (char*)malloc(sizeof(char) * (thirdcolon - secondcolon - 1));
    for(int i = 0; i < thirdcolon - secondcolon - 1; i++){
        size_str[i] = buffer[secondcolon + 1 + i];
    }
    size = atoi(size_str);

    // find forth segment of the buffer
    for(int i = thirdcolon + 1; i < BUFFER_SIZE; i++){
        if(buffer[i] == ':'){
            forthcolon = i;
            break;
        }
    }
    filename = (char*)malloc(sizeof(char) * (forthcolon - thirdcolon - 1));
    for(int i = 0; i < forthcolon - thirdcolon - 1; i++){
        filename[i] = buffer[thirdcolon + 1 + i];
    }

    // find final segment of the buffer
    for(int i = 0; i < size; i++){
        filedata[i] = buffer[forthcolon + 1 + i];
    }

    singlePacket->total_frag = total_frag;
    singlePacket->frag_no = frag_no;
    singlePacket->size = size;

    singlePacket->filename = (char *)malloc(sizeof(forthcolon - thirdcolon - 1));
    for(int i = 0; i < forthcolon - thirdcolon - 1; i++){
        singlePacket->filename[i] = filename[i];
    }
    for(int i = 0; i < size; i++){
        singlePacket->filedata[i] = filedata[i];
    }
}
