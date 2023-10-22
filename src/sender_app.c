#include "../include/sender_app.h"
#include "../include/link_layer.h"


unsigned char * getControlPacket(const unsigned int c, const char* filename, long int length, unsigned int *size){
    const int L1 = (length + 7) >> 3;;
    const int L2 = strlen(filename);
    
    *size = 1+2+L1+2+L2;

    
    unsigned char *packet = (unsigned char*)malloc(*size);

    
    unsigned int pos = 0;  
    packet[pos++]=c;
    packet[pos++]=0;
    packet[pos++]=L1;

    for (unsigned char i = 0 ; i < L1 ; i++) {
        packet[2+L1-i] = length & 0xFF;
        length >>= 8;
    }
    pos+=L1;
    packet[pos++]=1;
    packet[pos++]=L2;
    memcpy(packet+pos, filename, L2);
    return packet;

}

int sendFile(int serialPortFd, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        perror("File not found\n");
        exit(-1);
    }
    
    int file_init = ftell(file);
    fseek(file, 0L, SEEK_END);
    long int file_size = ftell(file)-file_init;
    rewind(file);


    unsigned int packet_size;
    
    unsigned char *controlPacketStart = getControlPacket(2, filename, file_size, &packet_size);
    if(llwrite(serialPortFd, controlPacketStart, packet_size) != 0){ 
        printf("Exit: error in start packet\n");
        return -1;
    }


    unsigned char sequence = 0;
    unsigned char* content = (unsigned char*)malloc(sizeof(unsigned char) * file_size);
    fread(content, sizeof(unsigned char), file_size, file);
    long int bytesLeft = file_size;


    while (bytesLeft >= 0) { 

        int dataSize = bytesLeft > (long int) MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : bytesLeft;
        unsigned char* data = (unsigned char*) malloc(dataSize);
        memcpy(data, content, dataSize);
        int psize;
        unsigned char* packet = getControlPacket(sequence, data, dataSize, &psize);
                
        if(llwrite(serialPortFd, packet, psize) == -1) {
            printf("Exit: error in data packets\n");
            exit(-1);
        }
                
        bytesLeft -= (long int) MAX_PAYLOAD_SIZE; 
        content += dataSize; 
        sequence = (sequence + 1) % 255;   
    }


    unsigned char *controlPacketEnd = getControlPacket(3, filename, file_size, &packet_size);
    if(llwrite(serialPortFd, controlPacketEnd, packet_size) == -1) { 
        printf("Exit: error in end packet\n");
        exit(-1);
    }
    return 0;
}