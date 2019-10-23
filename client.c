 /*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: hesiwei1
 *
 * Created on September 16, 2019, 11:11 PM
 */

/**
 * Part of the code is cited from https://beej.us/guide/bgnet/
 */

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

#include "packet.h"

void readFile(unsigned char **buffer, int *fileLen, char* fileSrc);
void createPacketList(struct packet*** packetList, int totalLength);
void fillInPacketList(struct packet** packetList, unsigned char* buffer, int totalLength, int remainingLen, char* fileName);
void byteCopy(unsigned char* buffer, int startPos, int endPos,unsigned char* Dest);
void freePacketList(struct packet** packetList, int totalLength);

int packetToStr(struct packet* packetPtr, unsigned char** packetStr); // TODO: NOT DONE!!!!
int intLen(unsigned int num);

/*
 * 
 */
int main(int argc, char** argv) {
    struct timeval start, end;
    double RRT;
    
    if(argc != 3){
        printf("[Error] Not enough or too many arguments.\n");
        return (EXIT_FAILURE);
    }
    char* destAddr = argv[1];
    char* portNum = argv[2];
    
    // ask for user input
    printf("Enter command:\n");
    
    char command[100];
    char fileSrc[100];
    
    scanf("%s", command);
    scanf("%s", fileSrc);
    
    if(strcmp(command, "ftp") != 0){
        printf("[Error] Only ftp is allowed.\n");
        return (EXIT_FAILURE);
    }else{
        // check if the file exists
        if( access( fileSrc, F_OK ) != -1 ) {
            printf("[INFO] File exists.\n");
        } else {
            printf("[Error] File not found.\n");
            return (EXIT_FAILURE);
        }
    }
    
    // socket()
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    
    // sendto()
    struct addrinfo hints;
    struct addrinfo* res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    int rv = getaddrinfo(destAddr, portNum, &hints, &res);
    
    gettimeofday(&start, NULL); // start clocking now
    
    int numbytes = sendto(s, "ftp", 3, MSG_CONFIRM, res->ai_addr, res->ai_addrlen);
    if (numbytes == -1){
        printf("[Error] Fail to send to server.\n");
        return (EXIT_FAILURE);
    }else {
        printf("[INFO] Success sending to server.\n");
    }
    
    // start receive info back from server
    
    // SET INIT TIMEOUT
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000; // 100ms
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Error");
    }
    
    // recvfrom()
    struct sockaddr_storage incoming_addr;
    socklen_t incoming_addr_size = sizeof incoming_addr;
    
    char buf[100];
//    numbytes = recvfrom(s, buf, 99, 0, (struct sockaddr *)&incoming_addr, &incoming_addr_size);
    if(recvfrom(s, buf, 99, 0, (struct sockaddr *)&incoming_addr, &incoming_addr_size) < 0){
        perror("recvfrom");
        printf("[Error] Server cannot be reached. Exit");
        return EXIT_FAILURE;
    }
    gettimeofday(&end, NULL); // stop clocking
//    long seconds = (end.tv_sec - start.tv_sec);
    long micros = (end.tv_usec) - (start.tv_usec);
    printf("[INFO] RRT : %lu us\n", micros); 
    
    // SET TIMEOUT BASED ON RRT
    tv.tv_sec = 0;
    tv.tv_usec = micros * 1000 * 10; // 10 * RRT
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Error");
    }
    
    buf[numbytes] = '\0';
    printf("[INFO] Receive Info from Server : %s\n", buf); 
    
    if(strcmp(buf, "yes") == 0){
        printf("[INFO] A file transfer can start\n");
    }
    
    // split to different packs
    int* fileLen = (int *)malloc(sizeof(int));
    unsigned char *buffer = (unsigned char *)malloc(sizeof(unsigned char));
    readFile(&buffer, fileLen, fileSrc);
    
    printf("[Info] File Length: %d\n", *fileLen);
    int complete_frag = int((*fileLen) / FRAG_SIZE);
    int remainingLen = *fileLen - complete_frag * FRAG_SIZE;
    int total_frag = (remainingLen == 0) ? complete_frag : (complete_frag + 1);
    printf("[Info] Total number of packets: %d;\n", total_frag);
    
    // construct packets
    struct packet** packetList;
    packetList = (packet**)malloc(sizeof(packet*) * total_frag);
    createPacketList(&packetList, total_frag);
    
    fillInPacketList(packetList, buffer, total_frag, remainingLen, fileSrc);
    
    // transform package to seperate strings
    for(int i = 0; i < total_frag; i++){
        unsigned char* charStr;
        int packageLen = packetToStr(packetList[i], &charStr);
        int sendTimes = 1;
                
        numbytes = sendto(s, charStr, packageLen, MSG_CONFIRM, res->ai_addr, res->ai_addrlen);
        if (numbytes == -1){
            printf("[Error] Fail to send to server.\n");
            return (EXIT_FAILURE);
        }else {
            printf("[INFO] Success sending to server. Packet: %d/%d \n", i + 1, total_frag);
        }
        
        // wait for acknowledgement
        char buf[100];

        while(recvfrom(s, buf, 99, 0, (struct sockaddr *)&incoming_addr, &incoming_addr_size) < 0 && sendTimes <= 5){
            perror("recvfrom");
            printf("Try %d-th times.\n", sendTimes);
            numbytes = sendto(s, charStr, packageLen, MSG_CONFIRM, res->ai_addr, res->ai_addrlen);
            if (numbytes == -1){
                printf("[Error] Fail to send to server.\n");
                return (EXIT_FAILURE);
            }else {
                printf("[INFO] Success sending to server. Packet: %d/%d \n", i + 1, total_frag);
            }
            
            sendTimes += 1;
        }
        
        if(sendTimes >= 6){
            printf("\n");
            printf("**************************************************************\n");
            printf("** [CRITICAL WARNING] Connection lost. Transfer Terminated. **\n");
            printf("**************************************************************\n");
            printf("\n");
            exit(0);
        }
        
        printf("[INFO] Server responded.\n");
    }
    
    // free all malloced space
    freePacketList(packetList, total_frag);
    free(buffer);
    free(fileLen);
    return (EXIT_SUCCESS);
}

void readFile(unsigned char **buffer, int* fileLen, char* fileSrc){
    // start read file
    FILE *file;
    file = fopen(fileSrc,"rb");
    
    // https://www.daniweb.com/programming/software-development/threads/188487/reading-binary-file-without-knowing-file-format
    //Get file length
    fseek(file, 0, SEEK_END);
    *fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate memory
    *buffer = (unsigned char *) malloc((*fileLen + 1) * sizeof(unsigned char));
    if (! *buffer) {
        fprintf(stderr, "Memory error!");
        fclose(file);
        exit(1);
    }

    // Read file contents into buffer
    fread(*buffer, *fileLen, 1, file);
    fclose(file);
}

void createPacketList(struct packet*** packetList, int totalLength){
    for(int i = 0; i < totalLength; i++){
        struct packet* singlePacket = (struct packet*)malloc(sizeof(struct packet));
        (*packetList)[i] = singlePacket;
    }
}

void fillInPacketList(struct packet** packetList, unsigned char* buffer, int totalLength, int remainingLen, char* fileName){
    int fileNameLen = strlen(fileName);
    printf("%d\n", fileNameLen);
    
    for(int i = 0; i < totalLength; i++){
        packetList[i]->total_frag = totalLength;
        packetList[i]->frag_no = i + 1;
        packetList[i]->size = (i == (totalLength - 1)) && (remainingLen != 0) ? remainingLen : FRAG_SIZE;
        packetList[i]->filename = (char*)malloc(sizeof(char) * (fileNameLen + 1));
        strcpy(packetList[i]->filename, fileName);
        
        if ((i == (totalLength - 1)) && (remainingLen != 0)) {
            byteCopy(buffer, i * FRAG_SIZE, remainingLen, packetList[i]->filedata);
        } else {
            byteCopy(buffer, i * FRAG_SIZE, FRAG_SIZE, packetList[i]->filedata);
        }
    }
}

void freePacketList(struct packet** packetList, int totalLength){
    for(int i = 0; i < totalLength; i++){
        free(packetList[i]->filename);
        free(packetList[i]);
    }
}

// startPos: start position of copy
// endPos: position immediately after end position of copy
void byteCopy(unsigned char* buffer, int startPos, int copyLen, unsigned char* Dest){
    for(int i = 0; i < copyLen; i++){
        Dest[i] = buffer[startPos + i];
    }
}

int packetToStr(struct packet* packetPtr, unsigned char** packetStr){
    int totalLength;
    
    int total_frag_length = intLen(packetPtr->total_frag);
    int frag_no_length = intLen(packetPtr->frag_no);
    int size_length = intLen(packetPtr->size);
    int filename_length = strlen(packetPtr->filename);
    
    totalLength = total_frag_length + frag_no_length + size_length + filename_length + packetPtr->size + 4;
    *packetStr = (unsigned char*)malloc(sizeof(unsigned char) * totalLength);
    
    // add total frag length
    char totalFlagStr[total_frag_length];
    sprintf(totalFlagStr, "%d", packetPtr->total_frag);
    for(int i = 0; i < total_frag_length; i++){
        (*packetStr)[i] = totalFlagStr[i];
    }
    (*packetStr)[total_frag_length] = ':';
    
    // add frag no
    char fragNoStr[frag_no_length];
    sprintf(fragNoStr, "%d", packetPtr->frag_no);
    for(int i = 0; i < frag_no_length; i++){
        (*packetStr)[total_frag_length + 1 + i] = fragNoStr[i];
    }
    (*packetStr)[total_frag_length + 1 + frag_no_length] = ':';
    
    // add frag no
    char sizeStr[size_length];
    sprintf(sizeStr, "%d", packetPtr->size);
    for(int i = 0; i < size_length; i++){
        (*packetStr)[total_frag_length + 1 + frag_no_length + 1+ i] = sizeStr[i];
    }
    (*packetStr)[total_frag_length + 1 + frag_no_length + 1 + size_length] = ':';
    
    // add file name
    for(int i = 0; i < filename_length; i++){
        (*packetStr)[total_frag_length + 1 + frag_no_length + 1 + size_length + 1 + i] = packetPtr->filename[i];
    }
    (*packetStr)[total_frag_length + 1 + frag_no_length + 1 + size_length + 1 + filename_length] = ':';
    
    // add content
    for(int i = 0; i < packetPtr->size; i++){
        (*packetStr)[total_frag_length + 1 + frag_no_length + 1 + size_length + 1 + filename_length + 1 + i] = packetPtr->filedata[i];
    }
    
    return totalLength;
}

int intLen(unsigned int num){
    int i = 0;
    while(num != 0){
        num /= 10;
        i++;
    }
    return i;
}