#include <pthread.h>
#include <stdlib.h>
#include "threads.h"
#include "shared.h"

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
void doublePoolSize(thread_pool_t *pool){
    size_t currentSize = pool->size*sizeof(thread_elm_t);
    pool->data = realloc((thread_elm_t*)pool->data, currentSize*2);
    if(pool->data == 0){
        Die("Could not Allocate memory for exta thead pool size");
    }
    pool->size = currentSize*2;
}

// ========================================================================= //