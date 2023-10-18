// Link layer protocol implementation

#include "../include/link_layer.h"


// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source


//global variables
int serialPortFd = -1;
int STOP_M = FALSE;
LinkLayerRole linkerRole;
int baudRate;
int timeout;
int numTries;
unsigned char byte;


enum State {START, FLAG, ADDR, CTRL, BCC1, ESCAPE};

enum State state;




int alarmEnabled = FALSE;
int alarmCount = 0;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = TRUE;
}



////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////


int llopen(LinkLayer connectionParameters)
{


    serialPortFd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);


    if (serialPortFd < 0) {
        perror("Error opening the serial port");
        return -1;
    }

    struct termios newtio;
    memset(&newtio, 0, sizeof(newtio));
    newtio.c_cflag = baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 1;  
    newtio.c_cc[VMIN] = 0;    
    tcflush(serialPortFd, TCIFLUSH);

    if (tcsetattr(serialPortFd, TCSANOW, &newtio) == -1) {
        perror("Error configuring serial port");
        close(serialPortFd);
        return -1;
    }


    linkerRole = connectionParameters.role;
    baudRate = connectionParameters.baudRate;
    timeout = connectionParameters.timeout; 
    numTries = connectionParameters.nRetransmissions;

    state = START;

    switch(linkerRole){
        (void)signal(SIGALRM, alarmHandler);
        // Writer Role
        case LlTx:
            while (numTries > 0) {
                if (alarmEnabled == TRUE) {
                    alarm(timeout);
                    alarmEnabled = FALSE;
                }
                unsigned char bytes[5] = {FLAG_BYTE, ADDR_SET, CTRL_SET, BCC1(ADDR_SET, CTRL_SET), FLAG_BYTE};
                int x = write(serialPortFd, bytes, 5);
                if (x == -1) {
                    perror("Error writing to the serial port");
                    return -1;
                }
                while(!STOP_M){
                    if (read(serialPortFd, &byte, 1)){
                        switch(state){
                            case START:
                                if (byte == FLAG_BYTE){
                                    state = FLAG;
                                }
                                break;
                            case FLAG:
                                if (byte == ADDR_UA){
                                    state = ADDR;
                                }
                                else if (byte != FLAG_BYTE){
                                    state = START;
                                }
                                break;
                            case ADDR:
                                if (byte == CTRL_UA){
                                    state = CTRL;
                                }
                                else if (byte == FLAG_BYTE){
                                    state = FLAG;
                                }
                                else{
                                    state = START;
                                }
                                break;
                            case CTRL:
                                if (byte == BCC1(ADDR_UA, CTRL_UA)){
                                    state = BCC1;
                                }
                                else if (byte == FLAG_BYTE){
                                    state = FLAG;
                                }
                                else{
                                    state = START;
                                }
                                break;
                            case BCC1:
                                if (byte == FLAG_BYTE){
                                    STOP_M = TRUE;
                                }
                                else{
                                    state = START;
                                }
                                break;
                            default:
                                break;
                        }
                    }
                }
                numTries--;
            }
            if (STOP_M != TRUE) return -1;
            break;


        // Reader Role
        case LlRx:
            while(!STOP_M){
                if (read(serialPortFd, &byte, 1)){
                    switch(state){
                        case START:
                            if (byte == FLAG_BYTE){
                                state = FLAG;
                            }
                            break;
                        case FLAG:
                            if (byte == ADDR_SET){
                                state = ADDR;
                            }
                            else if (byte != FLAG_BYTE){
                                state = START;
                            }
                            break;
                        case ADDR:
                            if (byte == CTRL_SET){
                                state = CTRL;
                            }
                            else if (byte == FLAG_BYTE){
                                state = FLAG;
                            }
                            else{
                                state = START;
                            }
                            break;
                        case CTRL:
                            if (byte == BCC1(ADDR_SET, CTRL_SET)){
                                state = BCC1;
                            }
                            else if (byte == FLAG_BYTE){
                                state = FLAG;
                            }
                            else{
                                state = START;
                            }
                            break;
                        case BCC1:
                            if (byte == FLAG_BYTE){
                                STOP_M = TRUE;
                            }
                            else{
                                state = START;
                            }
                            break;
                            default:
                                break;
                        }
                    }
                }
            }
    numTries = connectionParameters.nRetransmissions;
    return serialPortFd;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    if (serialPortFd < 0) {
        fprintf(stderr, "Serial port is not open\n");
        return -1;
    }

    int sequenceNumber = 0;

    int frame_len = bufSize + 6;
    unsigned char *frame = malloc(frame_len);
    
    memset(frame, 0, frame_len);

    frame[0] = FLAG_BYTE;
    frame[1] = ADDR_SET;
    frame[2] = (sequenceNumber << 6);
    frame[3] = BCC1(ADDR_SET, sequenceNumber);

    unsigned char bcc2 = 0;
    for (int i = 0; i < bufSize; i++)
    {
        frame[i + 4] = buf[i];
        bcc2 ^= buf[i];
    }
    
    int j = 4;
    int stuffingCount = 0;  // Counter for stuffed bytes

    for (unsigned int i = 0; i < bufSize; i++) {
        if (buf[i] == FLAG || buf[i] == ESCAPE_BYTE) {
            stuffingCount++;
        }
    }

    frame = realloc(frame, frame_len + stuffingCount);

    j = 4;  // Reset the position in the frame

    for (unsigned int i = 0; i < bufSize; i++) {
        if (buf[i] == FLAG || buf[i] == ESCAPE_BYTE) {
            frame[j++] = ESCAPE_BYTE;  // Insert an ESCAPE_BYTE before stuffed byte
            frame[j++] = buf[i] ^ 0x20;  // Stuff the byte
        }
        else{
            frame[j++] = buf[i];
        }        
    }

    // After processing the data, add BCC2 and the closing FLAG to the frame.
    frame[j++] = bcc2;
    frame[j++] = FLAG;

    int flag = 0;
    
    while (numTries > 0){
        if (alarmEnabled == TRUE) {
            alarm(timeout);
            alarmEnabled = FALSE;
        }
        int x = write(serialPortFd, frame, j);
        if (x == -1) {
            perror("Error writing to the serial port");
            return -1;
        }
        unsigned char byte;

        state = START;

        unsigned char ctrl_byte = 0;
        STOP_M = FALSE;

        while (STOP_M == FALSE && alarmEnabled == FALSE) {  
            if (read(serialPortFd, &byte, 1) > 0 == 1) {
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
                        if (byte == FLAG){
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

    return frame_len;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    if (serialPortFd < 0) {
        fprintf(stderr, "Serial port is not open\n");
        return -1;
    }

    unsigned char byte;

    //o que meter no tamanho do frame?
    unsigned char *data = malloc(FRAME_SIZE); // Allocate memory for the data
    unsigned char bcc1 = 0;
    unsigned char bcc2 = 0;
    int dataSize = 0;
    int state = START;

    while (state != END) {
        if (read(serialPortFd, &byte, 1) > 0) {
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
                    if (byte == CTRL_SET) {
                        state = CTRL;
                    } else if (byte == FLAG_BYTE) {
                        state = FLAG;
                    } else {
                        state = START;
                    }
                    break;
                case CTRL:
                    if (byte == BCC1(ADDR_SET, CTRL_SET)) {
                        bcc1 = byte;
                        state = BCC1;
                    } else if (byte == FLAG_BYTE) {
                        state = FLAG;
                    } else {
                        state = START;
                    }
                    break;
                case BCC1:
                    if (byte == FLAG_BYTE) {
                        state = END;
                    } else {
                        data[dataSize++] = byte; // Copy byte to data
                        bcc2 ^= byte; // BCC2 xor received byte
                        if (byte == ESCAPE_BYTE) { //next byte is stuffed
                            state = ESCAPE;
                        }
                    }
                    break;
                case ESCAPE:
                    data[dataSize++] = byte ^ 0x20; // Unstuff byte
                    bcc2 ^= byte ^ 0x20; // BCC2 xor unstuffed byte
                    state = BCC1; // Return to BCC1 state
                    break;
                default:
                    break;
            }
        }
    }

    free(data);

    if (bcc1 != BCC1(ADDR_SET, CTRL_SET) || bcc2 != 0) { // check if BCC1 = BCC1(ADDR_SET, CTRL_SET) and BCC2 = 0. If BCC2 != 0, there was an error in the data
        return -1;
    }

    // Perform byte destuffing
    int j = 0;
    for (int i = 0; i < dataSize; i++) { // Correctly copy the data to the packet and calculate how many chars were read
        if (data[i] == ESCAPE_BYTE) {
            packet[j++] = data[++i] ^ 0x20;
        } else {
            packet[j++] = data[i];
        }
    }

    return j;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    if (serialPortFd < 0) {
        fprintf(stderr, "Serial port is not open\n");
        return -1;
    }

    state = START;


    (void)signal(SIGALRM, alarmHandler);

    unsigned char bytes[5] = {FLAG_BYTE, ADDR_SET, CTRL_DISC, BCC1(ADDR_SET, CTRL_DISC), FLAG_BYTE};
    int x = write(serialPortFd, bytes, 5);
    if (x == -1) {
        perror("Error writing to the serial port");
        return -1;
    }
        
    while (numTries > 0) {
            if (alarmEnabled == TRUE)
            {
                alarm(timeout); // Set alarm to be triggered in s
                alarmEnabled = FALSE;
            }
            while(!STOP_M && !alarmEnabled){
                if (read(serialPortFd, &byte, 1)){
                    switch(state){
                        case START:
                            if (byte == FLAG_BYTE){
                                state = FLAG;
                            }
                            break;
                        case FLAG:
                            if (byte == ADDR_UA){
                                state = ADDR;
                            }
                            else if (byte != FLAG_BYTE){
                                state = START;
                            }
                            break;
                        case ADDR:
                            if (byte == CTRL_DISC){
                                state = CTRL;
                            }
                            else if (byte == FLAG_BYTE){
                                state = FLAG;
                            }
                            else{
                                state = START;
                            }
                            break;
                        case CTRL:
                            if (byte == BCC1(ADDR_UA, CTRL_DISC)){
                                state = BCC1;
                            }
                            else if (byte == FLAG_BYTE){
                                state = FLAG;
                            }
                            else{
                                state = START;
                            }
                            break;
                        case BCC1:
                            if (byte == FLAG_BYTE){
                                STOP_M = TRUE;
                            }
                            else{
                                state = START;
                            }
                            break;
                        default:
                            break;
                        }
                    }
                }
                numTries--;
            }
    unsigned char bytes_ua[5] = {FLAG_BYTE, ADDR_UA, CTRL_UA, BCC1(ADDR_UA, CTRL_UA), FLAG_BYTE};
    write(serialPortFd, bytes_ua, 5);
    
    if (close(serialPortFd) < 0) {
        perror("Error closing serial port");
        return -1;
    }

    if (showStatistics){
        printf("Statistics:\n"
               "  - Sent %d frames\n"
               "  - Received %d frames\n"
               "  - Rejected %d frames%d\n"
               "  - Timeouts: %d\n",
               0, 0, 0, 0);
    }

    return 1;
}
