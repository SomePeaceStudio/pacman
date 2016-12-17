#ifndef SHARED_H
#define SHARED_H

// ========================================================================= //
//                  CLIENT & SERVER SHARED FUNCTIONS
// ========================================================================= //

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define DEBUG 1
#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

void Die(char *mess);
void safeSend(int sockfd, const void *buf, size_t len, int flags);
void safeRecv(int sockfd, void *buf, size_t len, int flags);
char* allocPack(int size);
char receivePacktype(int sock);
int** allocateGameMap(int width, int height);
char* translateType(int type);
// For debugging on server/client
void printMap(int** map, int width, int height);
char* makeMapPack(int** map, int width, int height);
void printMappacPretty(char* mappac, int width, int height);
void printMappac(char* mappac, int width, int height);

#endif //SHARED_H