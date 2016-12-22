// ========================================================================= //
//                  CLIENT & SERVER SHARED FUNCTIONS
// ========================================================================= //

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h>
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
// ========================== THREAD POOL ================================== //
// ========================================================================= //

void initThreadPool(thread_pool_t *pool){
    // Pieprasām atmiņu priekš noklusētā daudzuma pavedienu
    pool->data = malloc(TPOOL_DEFAULT_SIZE*sizeof(thread_elm_t));
    if(pool->data == 0){
        Die("Could not Allocate memory for initial thead pool size");
    }
    for (int i = 0; i < TPOOL_DEFAULT_SIZE; ++i){
        pool->data[i].isFree = 1;
    }
    pool->size = TPOOL_DEFAULT_SIZE;
    pthread_mutex_init(&pool->mutex, 0);
}

// ------------------------------------------------------------------------- //

pthread_t getFreeThead(thread_pool_t *pool){
    // Atgriežam pirmo brīvo pavedienu
    pthread_mutex_lock(&pool->mutex);
    int i;
    for (i = 0; i < pool->size; ++i){
        if(pool->data[i].isFree == 1){
            pool->data[i].isFree = 0;
            pthread_mutex_unlock(&pool->mutex);
            return pool->data[i].thread;
        }
    }
    // Ja nav brīvu pavedienu palielinām pavedienu kopu
    doublePoolSize(pool);
    pool->data[i].isFree = 0;
    pthread_mutex_unlock(&pool->mutex);
    return pool->data[i].thread;
}

// ------------------------------------------------------------------------- //

// Tiek pieņemts, ka doublePoolSize() tiks izmantots tikai iekš mutex_lock
// TODO: Funkcija nestrādā ka vajaga, vairāk par 2x spēlētājiem 
// realloc(): invalid pointer errors.
void doublePoolSize(thread_pool_t *pool){
    size_t currentSize = pool->size*sizeof(thread_elm_t);
    pool->data = realloc(&(pool->data), currentSize*2);
    if(pool->data == 0){
        Die("Could not Allocate memory for exta thead pool size");
    }
    pool->size = currentSize*2;
}

// ========================================================================= //