#ifndef _SENDER_LINK_H_
#define _SENDER_LINK_H_


#include <stdio.h>

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(int serialPortFd, const unsigned char *buf, int bufSize, int timeout);

#endif // _SENDER_LINK_H_