#ifndef _SENDER_APP_H_
#define _SENDER_APP_H_


#include <stdio.h>


/**
 * Constructs a control packet for the file transmission.
 * Arguments:
 *   order: The order of the control packet (2 for start, 3 for end).
 *   filename: The name of the file to be transmitted.
 *   length: The length of the file in bytes.
 *   size: A pointer to where the size of the constructed packet will be stored.
 * Returns:
 *  A pointer to the constructed packet.
 */
unsigned char * getControlPacket(const unsigned int c, const char* filename, long int length, unsigned int *size);

/**
 * Sends a file over a serial port using the link layer protocol.
 * Arguments:
 *   serialPortFd: The file descriptor of the serial port to send the file over.
 *   filename: The name of the file to be sent.
 *   timeout: The time in seconds to wait for an acknowledgement before retransmitting a packet.
 *   nTries: The maximum number of times to attempt retransmission before giving up.
 * Returns: 
 *  0 if the file was sent successfully, -1 otherwise.
 */
int sendFile(int serialPortFd, const char* filename, int timeout, int nTries);

/**
 * Constructs a data packet from a given array of data.
 * Arguments:
 *   data: The data to be included in the packet.
 *   data_size: The size of the data in bytes.
 *   size: A pointer to where the size of the constructed packet will be stored.
 * Returns:
 *  A pointer to the constructed packet.
 */
unsigned char * getDataPacket(unsigned char* data, unsigned int data_size, unsigned int *size);

#endif // _SENDER_APP_H_