#ifndef _SENDER_APP_H_
#define _SENDER_APP_H_


#include <stdio.h>



unsigned char * getControlPacket(const unsigned int c, const char* filename, long int length, unsigned int *size);

int sendFile(int serialPortFd, const char* filename, int timeout, int nTries);

unsigned char * getDataPacket(unsigned char* data, unsigned int data_size, unsigned int *size);





#endif // _SENDER_APP_H_