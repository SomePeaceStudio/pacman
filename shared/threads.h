#ifndef THREADS_H
#define THREADS_H

// Nosaka cik daudz pavedieni tiks inicializēti iekš pavedienu kopas
#define TPOOL_DEFAULT_SIZE 5

typedef struct {
    pthread_t thread;
    short isFree;
} thread_elm_t;

typedef struct {
    volatile thread_elm_t* data;    // Saglabā norādi uz pavedienu kopas sākumu
    volatile size_t size;           // Nosaka pavedienu skaitu
    pthread_mutex_t mutex;          // Lai varētu droši palielināt pavedienu 
                                    // skaitu
    
} thread_pool_t;


void initThreadPool(thread_pool_t *pool);
pthread_t* getFreeThead(thread_pool_t *pool);
void doublePoolSize(thread_pool_t *pool);

#endif //THREADS_H