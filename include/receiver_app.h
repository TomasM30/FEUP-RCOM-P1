#ifndef _RECEIVER_APP_H_
#define _RECEIVER_APP_H_


#include <stdio.h>



///Receive a file over a serial port connection using the link layer protocol.
///Arguments:
// serialPortFd   The file descriptor of the serial port connection.
// Returns 0 on successful file reception, -1 on failure.
int receiveFile(int serialPortFd);


#endif // _RECEIVER_APP_H_