#include "../include/receiver_app.h"
#include "../include/link_layer.h"


unsigned char* getControlData(unsigned char* packet, int size, unsigned long int *fileSize) {

    // File Size
    unsigned char fileSizeNBytes = packet[2];
    unsigned char fileSizeAux[fileSizeNBytes];
    memcpy(fileSizeAux, packet+3, fileSizeNBytes);
    for(unsigned int i = 0; i < fileSizeNBytes; i++)
        *fileSize |= (fileSizeAux[fileSizeNBytes-i-1] << (8*i));

    // File Name
    unsigned char fileNameNBytes = packet[3+fileSizeNBytes+1];
    unsigned char *name = (unsigned char*)malloc(fileNameNBytes);
    memcpy(name, packet+3+fileSizeNBytes+2, fileNameNBytes);
    return name;
}


int receiveFile() {
    unsigned char *packet = (unsigned char *)malloc(MAX_PAYLOAD_SIZE);
    int packetSize = -1;
    while ((packetSize = llread(packet)) < 0);
        unsigned long int rxFileSize = 0;
        unsigned char* name = getControlData(packet, packetSize, &rxFileSize); 

        FILE* newFile = fopen((char *) name, "wb+");
        while (1) {    
            while ((packetSize = llread(packet)) < 0);
            if(packetSize == 0) break;
            else if(packet[0] != 3){
                unsigned char *buffer = (unsigned char*)malloc(packetSize);

                memcpy(buffer,packet+4,packetSize-4);
                buffer += packetSize+4;

                fwrite(buffer, sizeof(unsigned char), packetSize-4, newFile);
                free(buffer);
            }
            else continue;
        }
    if (fclose(newFile)!= 0) return -1;
    return 0;
}