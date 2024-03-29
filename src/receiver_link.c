#include "../include/receiver_link.h"
#include "../include/sender_link.h"
#include "../include/link_layer.h"



////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////

extern unsigned int sequenceNumber;

int llread(int serialPortFd, unsigned char *packet)
{
    if (serialPortFd < 0) {
        fprintf(stderr, "Serial port is not open receiver.c\n");
        return -1;
    }

    unsigned char byte;
    
    enum State state;

    state = START;
    int ctrl_byte = 0;
    int STOP_M = FALSE;
    
    int i = 0;

    

    while (STOP_M == FALSE) {
        int s = read(serialPortFd, &byte, 1);
        if (s) {
            switch (state) {
                case START:
                    if (byte == FLAG_BYTE) {
                        state = FLAG;
                    }
                     break;
                case FLAG:
                    if (byte == ADDR_SET) {
                        state = ADDR;
                    } else if (byte != FLAG_BYTE) {
                        state = START;
                    }
                    break;
                case ADDR:
                    if (byte == 0x00 || byte == 0x40) {
                        state = CTRL;
                        ctrl_byte = byte;
                        if (byte == 0x40 && sequenceNumber == 0) {
                            ctrl_byte = AcceptCtrlByteBySequenceNumber(sequenceNumber);
                            sendControlPacket(serialPortFd, ctrl_byte);
                            return -1;
                        }
                        if (byte == 0x00 && sequenceNumber == 1) {
                            ctrl_byte = AcceptCtrlByteBySequenceNumber(sequenceNumber);
                            sendControlPacket(serialPortFd, ctrl_byte);
                            return -1;
                        }

                    } else if (byte == FLAG_BYTE) {
                        state = FLAG;
                    } else if (byte == CTRL_DISC) {
                        return sendControlPacket(serialPortFd, ctrl_byte);
                    } else {
                        state = START;
                    }
                    break;
                case CTRL:
                    if (byte == BCC1(ADDR_SET, ctrl_byte)) {
                        state = READ_DATA;
                    } else if (byte == FLAG_BYTE) {
                        state = FLAG;
                    } else {
                        state = START;
                    }
                    break;
                case READ_DATA:
                    if(byte == FLAG_BYTE){
                        unsigned char bcc2 = packet[i-1];
                        i--;
                        packet[i] = '\0';
                        unsigned char bcc2_packet = generateBcc2(packet, i);
        
                        if (bcc2_packet == bcc2) {
                            STOP_M = TRUE;
                            ctrl_byte = AcceptCtrlByteBySequenceNumber(sequenceNumber);
                            
                            sendControlPacket(serialPortFd, ctrl_byte);
                            sequenceNumber = sequenceNumber^1;
                            return i;
                        }
                        else {
                            ctrl_byte = RejectCtrlByteBySequenceNumber(sequenceNumber);
                            sendControlPacket(serialPortFd, ctrl_byte);
                            return -1;
                        }
                        
                    }
                    else if (byte == ESCAPE_BYTE) {
                        state = ESCAPE;
                        
                    }
                    else{
                        packet[i++] = byte;
                    }
                    break;
                case ESCAPE:
                    state = READ_DATA;
                    packet[i++] = byte ^ 0x20;
                    
                    break;
                default:
                    break;
            }
        }
    }
    printf("=====rejected by timeout=====\n");
    return -1;
}

unsigned char generateBcc2(const unsigned char* data_rcv, int data_size) {
    unsigned char bcc2 = data_rcv[0];
    for (int i = 1; i < data_size; i++)
        bcc2 ^= data_rcv[i];
    return bcc2;
}



int AcceptCtrlByteBySequenceNumber(int sequenceNumber)
{
    if (sequenceNumber == 0) {
        return CTRL_RR1;
    }
    else {
        return CTRL_RR0;
    }
}

int RejectCtrlByteBySequenceNumber(int sequenceNumber)
{
    if (sequenceNumber == 0) {
        return CTRL_RJ1;
    }
    else {
        return CTRL_RJ0;
    }
}


int sendControlPacket(int serialPortFd, int ctrl_byte)
{
    unsigned char bytes[5]={FLAG_BYTE, ADDR_UA, ctrl_byte, BCC1(ADDR_UA, ctrl_byte), FLAG_BYTE};
    int x = write(serialPortFd, bytes, 5);
    if (x == -1) {
        perror("Error writing to the serial port");
        return -1;
    }
    return 0;
}
