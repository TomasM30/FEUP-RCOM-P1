#ifndef _SENDER_LINK_H_
#define _SENDER_LINK_H_


#include <stdio.h>

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(int serialPortFd, const unsigned char *packet, int packet_size, int timeout, int nTries);

#endif // _SENDER_LINK_H_