#ifndef _RECEIVER_LINK_H_
#define _RECEIVER_LINK_H_


#include <stdio.h>



// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(int serialPortFd, unsigned char *packet);

int AcceptCtrlByteBySequenceNumber(int sequenceNumber);

int RejectCtrlByteBySequenceNumber(int sequenceNumber);

int sendControlPacket(int serialPortFd, int ctrl_byte);

unsigned char generateBcc2(const unsigned char* data_rcv, int data_size);


#endif // _RECEIVER_LINK_H_