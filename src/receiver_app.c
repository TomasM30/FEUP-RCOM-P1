#include "../include/receiver_app.h"
#include "../include/link_layer.h"


int getControlData(unsigned char* packet, int size, unsigned long int *file_size) {

    unsigned char file_sizeNBytes = packet[2];
    unsigned char file_sizeAux[file_sizeNBytes];
    memcpy(file_sizeAux, packet+3, file_sizeNBytes);
    for(unsigned int i = 0; i < file_sizeNBytes; i++)
        *file_size |= (file_sizeAux[file_sizeNBytes-i-1] << (8*i));
    return 0;
}


int receiveFile(int serialPortFd) {
    
    unsigned char *packet = (unsigned char *)malloc(MAX_PAYLOAD_SIZE);
    int packet_size = -1;
    while ((packet_size =  llread(serialPortFd, packet)) < 0);
    unsigned long int read_size = 0;
    int conf = getControlData(packet, packet_size, &read_size); 
    if (conf != 0) return -1;

    printf("This is the Received File Size: %ld\n", read_size);

    FILE* file = fopen((char *) "penguin-received.gif", "wb+");
        
    while (read_size > 0) {
        int data_size = -1;
        while ((data_size = llread(serialPortFd, packet)) < 0);
        if (data_size == 0) break;
        else if (packet[0] == 1) {
            unsigned char *file_buffer = (unsigned char *)malloc(data_size - 3);
            printf("Data size: %d\n", data_size);
            memcpy(file_buffer, packet + 3, data_size - 3);
            for (int i = 0; i < data_size - 3; i++) {
                fputc(file_buffer[i], file);
            }
            free(file_buffer);
            } 
    }
    if (fclose(file)!= 0) return -1;
    else{
        printf("File Received\n");
    }
    return 0;
    
}