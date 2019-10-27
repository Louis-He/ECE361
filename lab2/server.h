#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "message.h"

// process incoming msg and return ack message type and
int processIncomingMsg(char* incomingMsg, unsigned char* ackInfo);

#endif
