#include "../include/receiver_link.h"
#include "../include/link_layer.h"



////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////

int llread(int serialPortFd, unsigned char *packet)
{
    if (serialPortFd < 0) {
        fprintf(stderr, "Serial port is not open receiver.c\n");
        return -1;
    }

    unsigned char byte;

    unsigned char bcc1 = 0;
    
    enum State state;

    state = START;
    int ctrl_byte = 0;
    int STOP_M = FALSE;
    
    int i = 0;

    int sequenceNumber = 0;

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
                        bcc1 = byte;
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
                            int x = sizeof(*packet);                            
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
                    if (byte == FLAG_BYTE || byte == ESCAPE_BYTE) packet[i++] = byte;
                    else{
                        packet[i++] = ESCAPE_BYTE;
                        packet[i++] = byte;
                    }
                    break;
                default:
                    break;
            }
        }
    }
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
        return CTRL_RR0;
    }
    else {
        return CTRL_RR1;
    }
}

int RejectCtrlByteBySequenceNumber(int sequenceNumber)
{
    if (sequenceNumber == 0) {
        return CTRL_RJ0;
    }
    else {
        return CTRL_RJ1;
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
