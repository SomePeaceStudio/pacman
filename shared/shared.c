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

void safeSend(int sockfd, const void *buf, size_t len, int flags){
    if (send(sockfd, buf, len, flags) != len) {
        Die("Mismatch in number of sent bytes");
    }
}

// ========================================================================= //

void safeRecv(int sockfd, void *buf, size_t len, int flags){
    int received;
    if ((received = recv(sockfd, buf, len, flags)) != len) {
        debug_print("Received: %2d bytes, should be: %d\n", received, (int)len);
        Die("Failed to receive bytes");
    }
    debug_print("Received: %2d bytes\n", received);
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
        printf("%s\n", "Error: Could not allocate memory");
        exit(1);
    }

    char **map = (char **)malloc( height * sizeof(char*) );
    if( map == NULL ){
        printf("%s\n", "Error: Could not allocate memory");
        exit(1);
    }
    for (int i = 0; i < height; i++)
        map[i] = &(data[width*i]);

    return map;
}

// ========================================================================= //

// Encoding
// 0 None
// 1 Dot
// 2 Wall
// 3 PowerPellet
// 4 Invincibility
// 5 Score

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