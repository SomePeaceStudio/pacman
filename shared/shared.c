// ========================================================================= //
//                  CLIENT & SERVER SHARED FUNCTIONS
// ========================================================================= //

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include "shared.h"

// ========================================================================= //

void Die(char *mess) { perror(mess); exit(1); }

// ========================================================================= //

int safeSend(int sockfd, const void *buf, size_t len, int flags){
    int sent;
    if ((sent = send(sockfd, buf, len, flags)) != len) {
        //debug_print("%s\n", ERR_SEND);
        perror(ERR_SEND);
        return sent;
    }
    return sent;
}

// ========================================================================= //

int safeRecv(int sockfd, void *buf, size_t len, int flags){
    int received;
    if ((received = recv(sockfd, buf, len, flags)) <= 0 ) {
        debug_print("%s\n","Failed to receive bytes");
        debug_print("Received: %2d bytes, asked-max: %d, flags used: %d\n", received,(int)len, flags);
        return received;
    }
    debug_print("Received: %2d bytes, asked-max: %d\n", received,(int)len);
    return received;
}
// ========================================================================= //

int serverRecv(int sockfd, void *buf, size_t len, int flags){
    int received;
    if ((received = recv(sockfd, buf, len, flags)) <= 0 ) {
        debug_print("%s\n","Failed to receive bytes");
        return received;
    }
    debug_print("Received: %2d bytes, asked-max: %d\n", received,(int)len);
    return received;
}

// ========================================================================= //

char* allocPack(int size){
    // Init pack with 0 byte
    char* pack = (char*)calloc(1, size);
    if( pack == NULL ){
        Die("Error: Could not allocate memory");
    }
    return pack;
}

// ========================================================================= //

char receivePacktype(int sock){
    char packtype;
    safeRecv(sock, &packtype, 1, 0);
    debug_print("Received packtype: %d\n", packtype);
    return packtype;
}

// ========================================================================= //

char** allocateGameMap(int width, int height){
    char *data = (char *)malloc( width * height );
    if( data == NULL ){
        printf(ERR_MALLOC);
        exit(1);
    }

    char **map = (char **)malloc( height * sizeof(char*) );
    if( map == NULL ){
        printf(ERR_MALLOC);
        exit(1);
    }
    for (int i = 0; i < height; i++)
        map[i] = &(data[width*i]);

    return map;
}

// ========================================================================= //

char* translateType(int type){
    if (type == -2){
        return ANSI_COLOR_BLUE "G" ANSI_COLOR_RESET;
    }
    if (type == -1){
        return ANSI_COLOR_RED "P" ANSI_COLOR_RESET;
    }
    if (type == 0){
        return "~";
    }
    if (type == 1){
        return ANSI_COLOR_YELLOW "." ANSI_COLOR_RESET;
    }
    if (type == 2){
        return "=";
    }
    if (type == 3){
        return ANSI_COLOR_CYAN "*" ANSI_COLOR_RESET;
    }
    if (type == 4){
        return ANSI_COLOR_MAGENTA "#" ANSI_COLOR_RESET;
    }
    if (type == 5){
        return ANSI_COLOR_GREEN "$" ANSI_COLOR_RESET;
    }
    return "E"; // E for Error
}

// ========================================================================= //

// For debugging on server/client
void printMap(char** map, int width, int height){
    for (int i = 0; i < height; ++i){
        for (int j = 0; j < width; ++j){
           printf(" %s", translateType(map[i][j]));
        }
        printf("\n");
    }
};

// ========================================================================= //

void printMappacPretty(char* mappac, int width, int height){
    for (int i = 0; i < width*height; ++i){
        printf(" %s", translateType(*(mappac+i)) );
        if((i+1) % width == 0){
            printf("\n");
        }        
    }
}

// ========================================================================= //

void printMappac(char* mappac, int width, int height){
    for (int i = 0; i < width*height; ++i){
        printf("%3d", *(mappac+i) );
        if((i+1) % width == 0){
            printf("\n");
        }        
    }
}

// ========================================================================= //

void printPacket(const unsigned char* packet, size_t length) {    
    printf("Packet data: ");
    for (int i = 0; i < length; i++) {
        printf("%x ", packet[i]);
    }
    
    printf("\n");
}

// ========================================================================= //

int32_t batoi(const unsigned char bytes[4]) {
    return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

// ========================================================================= //

void itoba (int32_t integer, unsigned char buffer[4]) {
    buffer[0] = (integer >> 24) & 0xFF;
    buffer[1] = (integer >> 16) & 0xFF;
    buffer[2] = (integer >> 8) & 0xFF;
    buffer[3] = integer & 0xFF;
}

// ========================================================================= //

float batof(const unsigned char bytes[4]) {
    int32_t integer = batoi(bytes);
    float* floatPtr = (float*)&integer;
    return *floatPtr;
}

// ========================================================================= //

void ftoba(float number, unsigned char buffer[4]) {
    int32_t* intPtr = (int32_t*)&number;
    itoba(*intPtr, buffer);
}

// ========================================================================= //

int clipMin(int number, int min) {
    if (number < min) return min;
    return number;
}

// ========================================================================= //

int clipMax(int number, int max) {
    if (number > max) return max;
    return number;
}

// ========================================================================= //

int clipBoth(int number, int min, int max) {
    //Ja nepareizs intervāls, tad samaina galapunktus vietām
    if (min > max) {
        int temp = min;
        min = max;
        max = temp;
    }
    
    number = clipMin(number, min);
    number = clipMax(number, max);
    return number;
}

// ========================================================================= //

// Nomaina visus x vērtības uz y vērtībām
void replaceChar(char* string, char x, char y){
    for(char* i = &string[0]; *i != '\0'; i++){
        if( *i == x ){
            *i = y;
        }
    }
}