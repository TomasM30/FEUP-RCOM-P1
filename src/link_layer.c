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

int alarmCount = 0;


int alarmEnabled = FALSE;

// Alarm function handler
void alarmHandler(int signal)
{
    printf("Alarm triggered\n");
    alarmEnabled = FALSE;
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
        // Writer Role
        case LlTx:
            (void)signal(SIGALRM, alarmHandler);
            while (numTries > 0 && STOP_M == FALSE) {
                if (alarmEnabled == FALSE) {
                    alarm(timeout);
                    alarmEnabled = TRUE;
                }

                unsigned char bytes[5] = {FLAG_BYTE, ADDR_SET, CTRL_SET, BCC1(ADDR_SET, CTRL_SET), FLAG_BYTE};
                int x = write(serialPortFd, bytes, 5);

                if (x == -1) {
                    perror("Error writing to the serial port");
                    return -1;
                }
                while(alarmEnabled && !STOP_M){
                    int s = read(serialPortFd, &byte, 1);
                    if (s){
                        switch(state){
                            case START:
                                if (byte == FLAG_BYTE){
                                    state = FLAG;                                }
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
                                    break;
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
            if (!numTries) printf("No more tries\n");
            if (STOP_M != TRUE) return -1;
            break;


        // Reader Role
        case LlRx:
            while(!STOP_M){
                int s = read(serialPortFd, &byte, 1);
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

    int STOP_M = FALSE;

    enum State state;

    state = START;

    if (serialPortFd < 0) {
        fprintf(stderr, "Serial port is not open\n");
        return -1;
    }

    switch(linkerRole){
        // Writer Role
        case LlTx:
        {
            (void)signal(SIGALRM, alarmHandler);
            while (numTries > 0 && STOP_M == FALSE) {
                if (alarmEnabled == FALSE) {
                    alarmHandler(timeout);
                    alarmEnabled = TRUE;
                }

                unsigned char bytes[5] = {FLAG_BYTE, ADDR_SET, CTRL_DISC, BCC1(ADDR_SET, CTRL_DISC), FLAG_BYTE};
                int x = write(serialPortFd, bytes, 5);

                if (x == -1) {
                    perror("Error writing to the serial port");
                    return -1;
                }
                while(alarmEnabled && !STOP_M){

                    int s = read(serialPortFd, &byte, 1);
                    if (s){
                        switch(state){
                            case START:
                                if (byte == FLAG_BYTE){
                                    state = FLAG;                                }
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
                                    alarmEnabled = FALSE;
                                    break;
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
            if (!numTries) printf("No more tries\n");
            if (STOP_M != TRUE) return -1;

            int send = sendControlPacket(serialPortFd, CTRL_UA);
            if (send == -1){
                printf("Error sending UA packet\n");
                return -1;
            }
            break;
        }
        // Reader Role
        case LlRx:
        {
            while(!STOP_M){
                int s = read(serialPortFd, &byte, 1);
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
                            if (byte == BCC1(ADDR_SET, CTRL_DISC)){
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

                int send = sendControlPacket(serialPortFd, CTRL_DISC);
                if (send == -1){
                    printf("Error sending UA packet\n");
                    return -1;
                }
        }
    }


    printf("Connection TERMINATED\n");
    return 0;
}