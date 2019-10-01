#ifndef PACKET_H
#define PACKET_H

#include <stdio.h>
#include <stdlib.h>

struct packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    unsigned char filedata[1000];
};

#endif