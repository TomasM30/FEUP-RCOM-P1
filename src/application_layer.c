// Application layer protocol implementation

#include "application_layer.h"
#include "receiver_app.h"
#include "sender_app.h"
#include "link_layer.h"
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>


int applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename) {

    LinkLayer linkLayer;
    strcpy(linkLayer.serialPort,serialPort);
    linkLayer.role = strcmp(role, "tx") ? LlRx : LlTx;
    linkLayer.baudRate = baudRate;
    linkLayer.nRetransmissions = nTries;
    linkLayer.timeout = timeout;      
    int serialPortFd = 0;
    serialPortFd = llopen(linkLayer);
    if(serialPortFd<0) return -1; 
    if (linkLayer.role == LlTx){
        printf("Sending file...\n");
        if (sendFile(serialPortFd, filename)!=0)
            return -1;
    }
    else if (linkLayer.role == LlRx){
        printf("Receiving file...\n");
        if (receiveFile()!=0)
            return -1;
    } 

    if (llclose(serialPortFd, 0) != 0) return -1;
    return 0;
}