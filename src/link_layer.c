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


enum State {START, FLAG, ADDR, CTRL, BCC1};

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
                if (alarmEnabled) {
                    alarm(timeout);
                }
                unsigned char bytes[5] = {FLAG_BYTE, ADDR_SET, CTRL_SET, BCC1(ADDR_SET, CTRL_SET), FLAG_BYTE};
                write(serialPortFd, bytes, 5);
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



    //int bytes = write(serialPortFd, frame, frameSize);
    //if (bytesn == -1) {
        //perror("Error writing to the serial port");
      //  return -1;
    //}

    return 1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
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
    write(serialPortFd, bytes, 5);
        
    while (numTries > 0) {
            if (alarmEnabled) {
                alarm(timeout);
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
