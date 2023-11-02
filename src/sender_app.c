#include "../include/sender_app.h"
#include "../include/link_layer.h"


unsigned char * getControlPacket(const unsigned int order, const char* filename, long int length, unsigned int *size){
    const int L1 = sizeof(length);
    const int L2 = strlen(filename);
    
    *size = 2+L1+2+L2+1;

    unsigned char *packet = (unsigned char*)malloc(*size);
    packet[0] = order;
    packet[1] = 0;
    packet[2] = L1;
    int pos = 3;
    for (unsigned char i = 0 ; i < L1 ; i++) {
        packet[2+L1-i] = length & 0xFF;
        length >>= 8;
    }
    pos+=L1;
    packet[pos++]= 1;
    packet[pos++] = L2;
    memcpy(packet+pos, filename, L2);

    return packet;
}


unsigned char * getDataPacket(unsigned char* data, unsigned int data_size, unsigned int *size){
    *size = data_size+3;
    unsigned char *packet = (unsigned char*)malloc(*size);
    packet[0] = 1;
    packet[1] = data_size/256;
    packet[2] = data_size%256;
    memcpy(packet+3, data, data_size);
    return packet;
}

int sendFile(int serialPortFd, const char* filename, int timeout, int nTries) {

    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("File not found\n");
        exit(-1);
    }
    
    int file_init = ftell(file);
    fseek(file, 0L, SEEK_END);
    int file_end = ftell(file);
    printf("File end: %d\n", file_end);
    long int file_size = file_end-file_init;
    rewind(file);

    printf("File size: %ld\n", file_size);


    unsigned int packet_size;

    
    unsigned char *startCtrlPacket = getControlPacket(2, filename, file_size, &packet_size);
    
    printf("====Sending Start CTRL Packet====\n");

    if(llwrite( serialPortFd, startCtrlPacket, packet_size, timeout, nTries) != 0){ 
        printf("Exit: error in start ctrl packet\n");
        return -1;
    }

    unsigned char* file_data = (unsigned char*)malloc(sizeof(unsigned char) * file_size);
    fread(file_data, sizeof(unsigned char), file_size, file);
    long int bytes_left = file_size;

    printf("====Sending Data Packets====\n");



    while (bytes_left >= 0) { 

        int data_size = bytes_left > (long int) MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : bytes_left;
        unsigned char* data = (unsigned char*) malloc(data_size);
        memcpy(data, file_data, data_size);
        unsigned int data_packet_size = 0;
        unsigned char* packet = getDataPacket(data, data_size, &data_packet_size);
                
        if(llwrite( serialPortFd, packet, data_packet_size, timeout, nTries) == -1) {
            printf("Exit: error in data packets\n");
            exit(-1);
        }
                
        bytes_left -= (long int) MAX_PAYLOAD_SIZE; 

        file_data += data_size; 

    }


    printf("====Sending Ending CTRL Packet====\n");

    unsigned char *endCtrlPacket = getControlPacket(3, filename, file_size, &packet_size);
    if(llwrite( serialPortFd, endCtrlPacket, packet_size, timeout, nTries) == -1) { 
        printf("Exit: error in end ctrl packet\n");
        exit(-1);
    }
    printf("=====File sent=====\n");
    
    return 0;
}