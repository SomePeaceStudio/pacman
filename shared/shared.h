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

// Pakešu tipi, kas definēti protokolā (PTYPE - Packet Type)
// Atbilst PacketType enumerācijai
#define PTYPE_JOIN 0
#define PTYPE_ACK 1
#define PTYPE_START 2
#define PTYPE_END 3
#define PTYPE_MAP 4
#define PTYPE_PLAYERS 5
#define PTYPE_SCORE 6
#define PTYPE_MOVE 7
#define PTYPE_MESSAGE 8
#define PTYPE_QUIT 9

// Pakešu izmēri tām paketēm, kurām ir fiksēti izmēri (PSIZE - Packet Size)
#define PSIZE_JOIN 21   // 20 baiti niks + 1 baits tipam
#define PSIZE_ACK 5     // Tips + spēlētāja id (int)
#define PSIZE_START 5   // Tips, map.height, map.size, player.x, player.y
#define PSIZE_END 1
#define PSIZE_SCORE 13  // Tips, packet.length, score, player.id
#define PSIZE_MOVE 6    // Tips, player.id, virziens
#define PSIZE_QUIT 5    // Tips, player.id

// Fiksēts spēlētāja objekta izmērs priekš PLAYERS tipa paketes
#define OSIZE_PLAYER 14 // id(int), x(float), y(float), PlayerState, PlayerType

// Virzieni, kādā spēlētāji var kustēties
// Atbilst protokola ClientMovement enumerācijai
#define DIR_UP 0
#define DIR_DOWN 1
#define DIR_RIGHT 2
#define DIR_LEFT 3

// Pielietots spēlētāja inicializēšana, norāda, ka spēlētājs nevirzās 
// nevienā virzienā
#define DIR_NONE -1

#define MAX_NICK_SIZE 20
// Paketes maksimālais izmērs, kad pakete tiek devinēta statiski
#define MAX_PACK_SIZE 1024


// Spēlētāja stāvoklis
#define PLSTATE_LIVE 0
#define PLSTATE_DEAD 1
#define PLSTATE_POWERUP 2

// Spēlētāja tips
#define PLTYPE_PACMAN 0
#define PLTYPE_GHOST 1

// Kartes objektu tipi
#define MTYPE_EMPTY 0
#define MTYPE_DOT   1
#define MTYPE_WALL  2
#define MTYPE_POWER 3
#define MTYPE_INVINCIBILITY 4
#define MTYPE_SCORE 5

//Teksti, ko varētu izmantot vairākās vietās
#define ERR_MALLOC "Failed to allocate memory"
#define ERR_SOCKET "Failed to create socket"
#define ERR_CONNECT "Failed to connect to socket"
#define ERR_BIND "Failed to bind socket"
#define ERR_RECV "Failed to receive bytes"
#define ERR_SEND "Failed to send bytes"

//Vispār pieņemti datu tipu izmēri
#define ENUM_SIZE 1
#define INT_SIZE 4


// ========================== STRUKTŪRAS =================================== //

typedef struct {
    char type;          // PLTYPE_ pacman vai ghost
    int id;             // Spēlētāja id
    char name[21];      // Spēlētāja vārds
    int points;         // Spēlētāja punkti (pacman) / kills (ghost) 
    float x;
    float y;
    char mdir;          // Kurstības virziens (move direction) glabā DIR_.. vērtību
    char state;         // PLSTATE_
    volatile int8_t disconnected;   // Globālais mainīgais, lai konstatētu, kad spēlētājs
                                    // ir atvienojies no servera
} object_t;

typedef struct {
    int tcp;
    int udp;
} sockets_t;

typedef struct {
    int socket;
    int32_t id;
} action_thread_para_t;

// ========================== PROTOTIPI ==================================== //

void Die(char *mess);
int safeSend(int sockfd, const void *buf, size_t len, int flags);
int safeRecv(int sockfd, void *buf, size_t len, int flags);
char* allocPack(int size);
char receivePacktype(int sock);
char** allocateGameMap(int width, int height);
char* translateType(int type);

// For debugging on server/client
void printMap(char** map, int width, int height);
void printMappacPretty(char* mappac, int width, int height);
void printMappac(char* mappac, int width, int height);
//Izdrukā paketes saturu kā HEX
void printPacket(const unsigned char* packet, size_t length);

//"Byte Array to Integer" un "Integer to Byte Array"
int32_t batoi (const unsigned char bytes[4]);
void itoba (int32_t integer, unsigned char buffer[4]);
//"Byte Array to Float" un "Float to Byte Array"
float batof(const unsigned char bytes[4]);
void ftoba(float number, unsigned char buffer[4]);

#endif //SHARED_H