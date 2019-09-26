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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "packet.h"

#define BACKLOG 10

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
    
    char buf[100];
    int numbytes = recvfrom(s, buf, 99, 0, (struct sockaddr *)&incoming_addr, &incoming_addr_size);
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
    
    return (EXIT_SUCCESS);
}
