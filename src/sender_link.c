#include "../include/sender_link.h"
#include "../include/link_layer.h"



////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(int serialPortFd, const unsigned char *packet, int packet_size, int timeout, int nTries)
{
    if (serialPortFd < 0) {
        fprintf(stderr, "Serial port is not open\n");
        return -1;
    }


    int sequenceNumber = 0;
    int frame_len = packet_size + 6;
    unsigned char *frame = malloc(frame_len);
    
    memset(frame, 0, frame_len);

    frame[0] = FLAG_BYTE;
    frame[1] = ADDR_SET;
    frame[2] = (sequenceNumber)?0x40:0x00; // 0x40 or 0x00
    frame[3] = BCC1(ADDR_SET, frame[2]);

    unsigned char bcc2 = 0;
    
    for (int i = 0; i < packet_size; i++)
    {
        frame[i + 4] = packet[i];
        bcc2 ^= packet[i];
    }
    
    
    int stuffingCount = 0;  // Counter for stuffed bytes

    for (unsigned int i = 0; i < packet_size; i++) {
        if (packet[i] == FLAG_BYTE || packet[i] == ESCAPE_BYTE) {
            stuffingCount++;
        }
    }

    frame = realloc(frame, frame_len + stuffingCount);

    
    int j = 4; // Reset the position in the frame to the first data byte
    for (unsigned int i = 0; i < packet_size; i++) {
        if (packet[i] == FLAG_BYTE || packet[i] == ESCAPE_BYTE) {
            frame[j++] = ESCAPE_BYTE;  // Insert an ESCAPE_BYTE before stuffed byte
            frame[j++] = packet[i] ^ 0x20;  // Stuff the byte
        }
        else{
            frame[j++] = packet[i];
        }        
    }

    // After processing the data, add BCC2 and the closing FLAG to the frame.
    if (bcc2 == FLAG_BYTE || bcc2 == ESCAPE_BYTE) {
        frame[j++] = ESCAPE_BYTE;
        frame[j++] = bcc2 ^ 0x20;
    }
    else{
        frame[j++] = bcc2;
    }
    frame[j++] = FLAG_BYTE;

    int alarmEnabled = TRUE;

    int numTries = timeout;

    (void)signal(SIGALRM, alarmHandler);

    printf("Sending frame...size: %d\n", sizeof(frame));

    int x = write(serialPortFd, frame, j);

        if (x == -1) {
            perror("Error writing to the serial port");
            return -1;
    }

    while (numTries > 0){
        if (alarmEnabled == TRUE) {
            alarm(timeout);
            alarmEnabled = FALSE;
        }
        unsigned char byte;

        enum State state;

        state = START;

        unsigned char ctrl_byte = 0;
        int STOP_M = FALSE;

        while (STOP_M == FALSE && alarmEnabled == FALSE) { 
            int s = read(serialPortFd, &byte, 1); 
            if (s) {
                switch (state) {
                    case START:
                        if (byte == FLAG_BYTE) state = FLAG;
                        break;
                    case FLAG:
                        if (byte == ADDR_UA) state = ADDR;
                        else if (byte != FLAG_BYTE) state = START;
                        break;
                    case ADDR:
                        if (byte == CTRL_RR0 || byte == CTRL_RR1 || byte == CTRL_RJ0 || byte == CTRL_RJ1 || byte == CTRL_DISC){
                            state = CTRL;
                            ctrl_byte = byte;   
                        }
                        else if (byte == FLAG_BYTE) state = FLAG;
                        else state = START;
                        break;
                    case CTRL:
                        if (byte == BCC1(ADDR_UA,ctrl_byte)) state = BCC1;
                        else if (byte == FLAG_BYTE) state = FLAG;
                        else state = START;
                        break;
                    case BCC1:
                        if (byte == FLAG_BYTE){
                            STOP_M = TRUE;
                        }
                        else state = START;
                        break;
                    default: 
                        break;
            }
        } 
    } 

        if (ctrl_byte == CTRL_RR0 || ctrl_byte == CTRL_RR1){
            if (sequenceNumber == 0){
                sequenceNumber = 1;
            }
            else{
                sequenceNumber = 0;
            }
            break;
        } else if (ctrl_byte == CTRL_RJ0 || ctrl_byte == CTRL_RJ1){
            numTries--;
            continue;
        } else{
            continue;
        }

    }
   


    free(frame);

    return 0;
}
