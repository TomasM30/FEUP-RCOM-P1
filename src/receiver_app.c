#include "../include/receiver_app.h"
#include "../include/link_layer.h"


unsigned char* getControlData(unsigned char* packet, int size, unsigned long int *file_size) {

    unsigned char file_sizeNBytes = packet[2];
    unsigned char file_sizeAux[file_sizeNBytes];
    memcpy(file_sizeAux, packet+3, file_sizeNBytes);
    for(unsigned int i = 0; i < file_sizeNBytes; i++)
        *file_size |= (file_sizeAux[file_sizeNBytes-i-1] << (8*i));


    unsigned char fileNameNBytes = packet[3+file_sizeNBytes+1];
    unsigned char *name = (unsigned char*)malloc(fileNameNBytes);
    memcpy(name, packet+3+file_sizeNBytes+2, fileNameNBytes);
    return name;
}


int receiveFile(int serialPortFd) {
    
    unsigned char *packet = (unsigned char *)malloc(MAX_PAYLOAD_SIZE);
    int packet_size = llread(serialPortFd, packet);
    printf("Packet size: %d\n", packet_size);
    while (packet_size < 0);
        unsigned long int read_size = 0;
        unsigned char* name = getControlData(packet, packet_size, &read_size); 

        FILE* file = fopen((char *) "penguin-received.gif", "wb+");
        
        while (1) {    
            
            while ((packet_size = llread(serialPortFd, packet)) < 0);
            if(packet_size == 0) break;
            else if(packet[0] != 3){
                
                unsigned char *file_buffer = (unsigned char*)malloc(packet_size);
                memcpy(file_buffer,packet+4,packet_size-4);
                file_buffer += packet_size+4;

                

                for (int i = 0; i < packet_size; i++){
                    fputc(packet[i+3], file);
                }
                
                free(file_buffer);
                
            }
            else continue;
        }
    if (fclose(file)!= 0) return -1;
    return 0;
}