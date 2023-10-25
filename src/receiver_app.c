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
        while ((packet_size =  llread(serialPortFd, packet)) < 0){
                printf("Error reading data packet\n");
        }
        unsigned long int read_size = 0;
        int conf = getControlData(packet, packet_size, &read_size); 
        if (conf != 0) return -1;

        printf("This is the Received File Size: %ld\n", read_size);

        FILE* file = fopen((char *) "penguin-received2.gif", "wb+");
        
        while (1) {    
            
            int data_size = -1;
            while ((data_size = llread(serialPortFd, packet)) < 0){
                printf("Error reading data packet\n");
            }

            printf("Reading %d BYTES \n", data_size);
            
            if(data_size == 0) break;
            else if(packet[0] != 3){ 
                
                unsigned char *file_buffer = (unsigned char*)malloc(data_size);
                memcpy(file_buffer, packet+3, data_size-3);
                file_buffer += data_size+3;                

                for (int i = 0; i < data_size; i++) {
                    fputc(file_buffer[3+i], file);
                }
                
                
                free(*file_buffer);
                
            }
            else continue;
        }

    
    if (fclose(file)!= 0) return -1;
    return 0;
}