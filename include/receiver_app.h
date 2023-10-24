#ifndef _RECEIVER_APP_H_
#define _RECEIVER_APP_H_


#include <stdio.h>



int receiveFile(int serialPortFd);


unsigned char* getControlData(unsigned char* packet, int size, unsigned long int *fileSize);



#endif // _RECEIVER_APP_H_