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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <sys/time.h>

#include "packet.h"

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
    // recvfrom()
    struct sockaddr_storage incoming_addr;
    socklen_t incoming_addr_size = sizeof incoming_addr;
    
    char buf[100];
    numbytes = recvfrom(s, buf, 99, 0, (struct sockaddr *)&incoming_addr, &incoming_addr_size);
    if(numbytes == -1){
        perror("recvfrom");
    }
    gettimeofday(&end, NULL); // stop clocking
//    long seconds = (end.tv_sec - start.tv_sec);
    long micros = (end.tv_usec) - (start.tv_usec);
    printf("[INFO] RRT : %lu us\n", micros); 
    
    buf[numbytes] = '\0';
    printf("[INFO] Receive Info from Server : %s\n", buf); 
    
    if(strcmp(buf, "yes") == 0){
        printf("[INFO] A file transfer can start\n");
    }
    
    return (EXIT_SUCCESS);
}
