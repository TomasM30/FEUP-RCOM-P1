#ifndef _RECEIVER_APP_H_
#define _RECEIVER_APP_H_


#include <stdio.h>



/** Receive a file over a serial port connection using the link layer protocol.
 * Arguments:
 * serialPortFd: The file descriptor of the serial port connection.
 * Returns:
 *  0 on successful file reception, -1 on failure.
 */
int receiveFile(int serialPortFd);

/** Get the control data from the packet.
 * Arguments:
 * packet: The packet from which to get the control data.
 * size: The size of the packet.
 * file_size: Pointer to a variable where the file size will be stored.
 * Returns:
 *  0 on success, -1 on failure.
 */
int getControlData(unsigned char* packet, int size, unsigned long int *file_size);


#endif // _RECEIVER_APP_H_