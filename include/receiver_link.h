#ifndef _RECEIVER_LINK_H_
#define _RECEIVER_LINK_H_


#include <stdio.h>


/**
 * Reads a packet of data from a serial port using the link layer protocol.
 * Arguments:
 *   serialPortFd: The file descriptor of the serial port to read from.
 *   packet: A pointer to the buffer where the read data will be stored.
 * Returns:
 *   The size of the read packet on success, -1 on failure or if a repeated packet is detected.
 */
int llread(int serialPortFd, unsigned char *packet);

/**
 * Generates the BCC2 for a given array of data.
 * Arguments:
 *   data_rcv: The data to generate the BCC2 for.
 *   data_size: The size of the data in bytes.
 * Returns:
 *  The BCC2 for the given data.
 */
unsigned char generateBcc2(const unsigned char* data_rcv, int data_size);

/**
 * Determines the control byte to send in an acknowledgement for a received packet.
 * Arguments:
 *   sequenceNumber: The sequence number of the received packet.
 * Returns:
 *  The control byte to send in an acknowledgement for a received packet.
 */
int AcceptCtrlByteBySequenceNumber(int sequenceNumber);

/**
 * Determines the control byte to send in a rejection for a received packet.
 * Arguments:
 *   sequenceNumber: The sequence number of the received packet.
 * Returns:
 *  The control byte to send in a rejection for a received packet.
 */
int RejectCtrlByteBySequenceNumber(int sequenceNumber);

/**
 * Sends a control packet over a serial port.
 * Arguments:
 *   serialPortFd: The file descriptor of the serial port to send the control packet over.
 *   ctrl_byte: The control byte to include in the control packet.
 * Returns:
 *  0 if the control packet was sent successfully, -1 otherwise.
 */
int sendControlPacket(int serialPortFd, int ctrl_byte);

#endif // _RECEIVER_LINK_H_