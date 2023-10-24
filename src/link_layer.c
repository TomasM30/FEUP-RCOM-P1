// Link layer protocol implementation

#include "../include/link_layer.h"
#include "../include/receiver_link.h"


// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source


//global variables
LinkLayerRole linkerRole;
int baudRate;
int timeout;
int numTries;
unsigned char byte;


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
    int serialPortFd = -1;

    serialPortFd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);

    printf("%d\n", connectionParameters.role);


    if (serialPortFd < 0) {
        perror("Error opening the serial port");
        return -1;
    }

    else{
        printf("Opening serial port: %s\n", connectionParameters.serialPort);
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

    enum State state;

    state = START;

    int STOP_M = FALSE;


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
                    int s = read(serialPortFd, &byte, 1);
                    if (s){
                        switch(state){
                            case START:
                                if (byte == FLAG_BYTE){
                                    state = FLAG;
                                    printf("FLAG\n");
                                }
                                break;
                            case FLAG:
                                if (byte == ADDR_UA){
                                    state = ADDR;
                                    printf("ADDR\n");
                                }
                                else if (byte != FLAG_BYTE){
                                    state = START;
                                    printf("START\n");
                                }
                                break;
                            case ADDR:
                                if (byte == CTRL_UA){
                                    state = CTRL;
                                    printf("CTRL\n");
                                }
                                else if (byte == FLAG_BYTE){
                                    state = FLAG;
                                    printf("FLAG\n");
                                }
                                else{
                                    state = START;
                                    printf("START\n");
                                }
                                break;
                            case CTRL:
                                if (byte == BCC1(ADDR_UA, CTRL_UA)){
                                    state = BCC1;
                                    printf("BCC1\n");
                                }
                                else if (byte == FLAG_BYTE){
                                    state = FLAG;
                                    printf("FLAG\n");
                                }
                                else{
                                    state = START;
                                    printf("START\n");
                                }
                                break;
                            case BCC1:
                                if (byte == FLAG_BYTE){
                                    STOP_M = TRUE;
                                    printf("STOP\n");
                                }
                                else{
                                    state = START;
                                    printf("START\n");
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
                int s = read(serialPortFd, &byte, 1);
                printf("bytes read: %d\n", s);
                if (s){
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

                int send = sendControlPacket(serialPortFd, CTRL_UA);
                if (send == -1){
                    printf("Error sending UA packet\n");
                    return -1;
                }
            }
    numTries = connectionParameters.nRetransmissions;


    printf("Connection established\n");
    return serialPortFd;
}



////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int serialPortFd, int showStatistics)
{
    if (serialPortFd < 0) {
        fprintf(stderr, "Serial port is not open\n");
        return -1;
    }

    enum State state = START;


    (void)signal(SIGALRM, alarmHandler);

    unsigned char bytes[5] = {FLAG_BYTE, ADDR_SET, CTRL_DISC, BCC1(ADDR_SET, CTRL_DISC), FLAG_BYTE};
    int x = write(serialPortFd, bytes, 5);
    if (x == -1) {
        perror("Error writing to the serial port");
        return -1;
    }
    int STOP_M = FALSE;

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

    return 0;
}
