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

//Pakešu tipi, kas definēti protokolā (PTYPE - Packet Type)
//Atbilst PacketType enumerācijai
#define PTYPE_JOIN 0
#define PTYPE_ACK 1
#define PTYPE_START 2
#define PTYPE_END 3
#define PTYPE_MAP 4
#define PTYPE_PLAYERS 5
#define PTYPE_SCORE 6
#define PTYPE_MOVE 7

//Pakešu izmēri tām paketēm, kurām ir fiksēti izmēri (PSIZE - Packet Size)
#define PSIZE_JOIN 21 //20 baiti niks + 1 baits tipam
#define PSIZE_ACK 5 //Tips + spēlētāja id (int)
#define PSIZE_START 5 //Tips, map.height, map.size, player.x, player.y
#define PSIZE_END 1
#define PSIZE_SCORE 13 //Tips, packet.length, score, player.id
#define PSIZE_MOVE 6 //Tips, player.id, virziens

//Virzieni, kādā spēlētāji var kustēties
//Atbilst protokola ClientMovement enumerācijai
#define DIR_UP 0
#define DIR_DOWN 1
#define DIR_RIGHT 2
#define DIR_LEFT 3

#define MAX_NICK_SIZE 20

typedef struct {
    char type;
    int id;
    double x;
    double y;
    int status;
} object_t;

void Die(char *mess);
void safeSend(int sockfd, const void *buf, size_t len, int flags);
void safeRecv(int sockfd, void *buf, size_t len, int flags);
char* allocPack(int size);
char receivePacktype(int sock);
char** allocateGameMap(int width, int height);
char* translateType(int type);
// For debugging on server/client
void printMap(char** map, int width, int height);
char* makeMapPack(int** map, int width, int height);
void printMappacPretty(char* mappac, int width, int height);
void printMappac(char* mappac, int width, int height);

#endif //SHARED_H