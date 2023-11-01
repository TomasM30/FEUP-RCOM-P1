#ifndef _SENDER_LINK_H_
#define _SENDER_LINK_H_


#include <stdio.h>


/**
 * Writes a packet of data to a serial port using the link layer protocol.
 * Arguments:
 *   sequenceNumber: The sequence number of the packet to be sent. This should alternate between 0 and 1 for each packet sent.
 *   serialPortFd: The file descriptor of the serial port to write to.
 *   packet: The data packet to be sent.
 *   packet_size: The size of the data packet in bytes.
 *   timeout: The time in seconds to wait for an acknowledgement before retransmitting the packet.
 *   nTries: The maximum number of times to attempt retransmission before giving up.
 */
int llwrite(unsigned int sequenceNumber, int serialPortFd, const unsigned char *packet, int packet_size, int timeout, int nTries);


#endif // _SENDER_LINK_H_