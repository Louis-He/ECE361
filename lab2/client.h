#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMAND_LEN 1000

// return status:
// 0: quit
// 1: ready to transfer
// 2: error input
int readInAndProcessCommand(unsigned char* commandLine[5], unsigned char* encodedData);
// int dataToStr(unsigned char* commandLine[5], unsigned char* dataField, int commandArgc);
void unsignedStrCopy(unsigned char* dst, unsigned char* src);
