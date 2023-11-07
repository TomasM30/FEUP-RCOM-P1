#include "../include/sender_link.h"
#include "../include/link_layer.h"

unsigned int sequenceNumber;

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(int serialPortFd, const unsigned char *packet, int packet_size, int timeout, int nTries)
{
    if (serialPortFd < 0) {
        fprintf(stderr, "Serial port is not open\n");
        return -1;
    }


    
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

    extern int alarmEnabled;

    int numTries = nTries;

    unsigned char byte;

    enum State state;

    state = START;

    unsigned char ctrl_byte = 0;
    
    int STOP_M = FALSE;

    (void)signal(SIGALRM, alarmHandler);
    while (numTries > 0 && STOP_M == FALSE){
        if (alarmEnabled == FALSE) {
            alarm(timeout);
            alarmEnabled = TRUE;
        }
        int x = write(serialPortFd, frame, j);
        //sleep(1);
        printf("writinnnnnnng: %d\n", j);
        if (x == -1) {
            perror("Error writing to the serial port");
            return -1;
        }

        while (alarmEnabled && !STOP_M) { 
            int s = read(serialPortFd, &byte, 1); 
            printf("TESTING %d\n", s);
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
                        if (byte == BCC1(ADDR_UA,ctrl_byte)){
                            state = BCC1;
                        } 
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
         
        printf("READ\n");
        if (ctrl_byte == 0) continue;
        
        if ((sequenceNumber == 0 && ctrl_byte == CTRL_RR1) || 
        (sequenceNumber == 1 && ctrl_byte == CTRL_RR0))
        {
            printf("Exit: success in llwrite\n");
            sequenceNumber = (sequenceNumber + 1) % 2;
            return 0;
        } else {
            STOP_M = FALSE;
            continue;
        }
        numTries--;
    }  
    

    return -1;
}
    
