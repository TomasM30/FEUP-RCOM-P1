// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#define FLAG_BYTE (0X7E)
#define ESCAPE_BYTE (0X7D)
#define ADDR_SET (0X03)
#define ADDR_UA (0X01)
#define CTRL_SET (0X03)
#define CTRL_UA (0X07)
#define CTRL_DISC (0x0B)
#define CTRL_RR0 (0X05)
#define CTRL_RR1 (0x85)
#define CTRL_RJ0 (0X01)
#define CTRL_RJ1 (0x81)
#define BCC1(a,c) (a^c)

enum State {START, FLAG, ADDR, CTRL, BCC1, READ_DATA, ESCAPE};


typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;



void alarmHandler(int signal);


// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

// MISC
#define FALSE 0
#define TRUE 1

// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(LinkLayer connectionParameters);



// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console on close.
// Return "1" on success or "-1" on error.
int llclose(int serialPortFd, int showStatistics);

#endif // _LINK_LAYER_H_
