#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <math.h>
#include <pthread.h>


// #define BUFFSIZE 32
#define MAXNICKSIZE 20
#define DEBUG 1
#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// ------------------------------------------------------------------------- //

typedef struct {
    char type;
    char response;
    int id;
} join_t;

typedef struct {
    char type;
    int id;
    double x;
    double y;
    int status;
} object_t;


int **MAP;
object_t **MAP2;

int MAPWIDTH;
int MAPHEIGHT;
int playerId;
char playerName[MAXNICKSIZE+1] = "pacMonster007----END";
pthread_t  tid;   // Second thread
//tcb- theard control block.;



// ========================================================================= //

void Die(char *mess) { perror(mess); exit(1); }
void safeSend(int sockfd, const void *buf, size_t len, int flags);
void safeRecv(int sockfd, void *buf, size_t len, int flags);
int** allocateGameMap(int width, int height);
object_t** allocateGameMap2(int width, int height);
void printMappac();
void printMap();
void convertMappacToArray(char* mappac);
void convertMappacToArray2(char* mappac);
void updateMap();
void initializeMap();
char* translateType(int type);
int joinGame(int sock);
char* allocPack(int size);
int joinGame(int sock);

void* actionTherad();   /* Thread function to client actions */

// ========================================================================= //

void* actionTherad(void *parm){
    sleep(3);
    int sock = (int)(intptr_t)parm;
    while(1){
        printf("%s\n", "a: <- left, w: up, d: -> right, s: down");
        char* move;

        fscanf(stdin,"%s", move);
        // Get vector
        int vector[2] = {0,0};
        if(move[0] == 'a'){
            vector[1] = -1;
        }else if( move[0] == 'w' ){
            vector[0] = -1;
        }else if( move[0] == 'd' ){
            vector[1] = 1;
        }else if( move[0] == 's' ){
            vector[0] = 1;
        }
        // Wrong   
        if( vector[0] == 0 && vector[1] == 0 ){
            continue;
        }
        /* Send move */
        int packSize = sizeof(char)+sizeof(int)*3;
        char* pack = (char*)malloc(packSize);
        pack[0] = 'A';
        pack[1] = playerId;
        memcpy(&pack[5],&vector, sizeof(vector));
        debug_print("%s\n", "Sending...");
        if (send(sock, pack, packSize, 0) != packSize) {
            Die("Mismatch in number of sent bytes");
        }
        printf("You'r move was: %c\n", move[0]);
    }
};

void printAncii(){
    int i;
    i=0;
    do
    {
        printf("%d %c \n",i,i);
        i++;
    }
    while(i<=255);
    return;
}

void printObj(object_t obj){
    printf("ID: %1d Type: %c X: %2f Y: %2f ST: %d\n",\
        obj.id, obj.type, obj.x, obj.y, obj.status);
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in gameserver;
    char packtype;

    if (argc != 3) {
      fprintf(stderr, "USAGE: client <server_ip> <port>\n");
      exit(1);
    }

    /* Create the TCP socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      Die("Failed to create socket");
    }

    /* Construct the server sockaddr_in structure */
    memset(&gameserver, 0, sizeof(gameserver));       /* Clear struct */
    gameserver.sin_family = AF_INET;                  /* Internet/IP */
    gameserver.sin_addr.s_addr = inet_addr(argv[1]);  /* IP address */
    gameserver.sin_port = htons(atoi(argv[2]));       /* server port */

    /* Establish connection */
    if (connect(sock,
                (struct sockaddr *) &gameserver,
                sizeof(gameserver)) < 0) {
      Die("Failed to connect with server");
    }
    
    joinGame(sock);
    return 0;
    /* Try to join game */
    // packtype = 0;
    // if (send(sock, &type, sizeof(type), 0) != sizeof(type)) {
    //     Die("Mismatch in number of sent bytes");
    // }

    // join_t jbuffer;
    // /* Receive the id from the server */
    // if (recv(sock, &jbuffer, sizeof(jbuffer), 0) < 1) {
    //     Die("Failed to receive bytes from server");
    // }
    // playerId = jbuffer.id;
    // printf("Type: %c Response: %c Id: %d\n", jbuffer.type, jbuffer.response, jbuffer.id);
    
    //Receive stuff from a server
    pthread_create(&tid, NULL, actionTherad, (void*)(intptr_t) sock);   
    while(1){
        char pactype = '0';
        /* Receive message type*/
        debug_print("%s\n", "Receiving packtype...");
        if (recv(sock, &pactype, sizeof(pactype), 0) < 0) {
            Die("Failed to receive packtype from server");
        }
        debug_print("Received pactype: %c\n", pactype);

        if(pactype == 'U'){
            /* Receive the map size from the server */
            int received = 1; 
            debug_print("%s\n", "Receiving pac size...");
            int updateSize;
            if (recv(sock, &updateSize, sizeof(updateSize), 0) < 1) {
                Die("Failed to receive updateSize from server");
            }
            debug_print("Received map object count: %2d\n", updateSize);
            received += sizeof(updateSize);
            for(int i = 0; i < updateSize; ++i){
                object_t objectBuffer;
                debug_print("Receiving %d. obj.\n", i+1);
                if (recv(sock, &objectBuffer, sizeof(object_t), 0) < 1) {
                    Die("Failed to receive updateSize from server");
                }
                printObj(objectBuffer);
                updateMap(objectBuffer);
                debug_print("%s\n", "Updated Map:");
                //printMap();
                received += sizeof(object_t);
                debug_print("Received %d bytes.\n", received);
            }
            printMap();
            continue;
        }

        if(pactype == 'M'){
            /* Receive the map size from the server */
            debug_print("%s\n", "Receiving map height/widht...");
            char* mapsize = malloc(sizeof(int)*2);
            if (recv(sock, mapsize, sizeof(int)*2, 0) < 1) {
                Die("Failed to receive mapsize from server");
            }
            debug_print("Received Map width: %d\n", mapsize[0]);
            debug_print("Received Map height: %d\n", mapsize[4]);

            MAPWIDTH = (int)mapsize[0];
            MAPHEIGHT = (int)mapsize[4];
            initializeMap();

            //Unused now...
            continue;
            // int msize = sizeof(int)*MAPWIDTH*MAPHEIGHT;
            // debug_print("Map size: %d\n", msize);

            // /* Receive the map from the server */
            // char* map = malloc(msize);
            // debug_print("%s\n", "Receiving...");
            // if (recv(sock, map, msize, 0) < 1) {
            //     Die("Failed to receive map from server");
            // }
            // convertMappacToArray(map);
            // printMap();
        }
        // Connection has been lost
        if(pactype == '0'){
            break;
        }   
    }

    fprintf(stdout, "\n");
   	close(sock);
   	exit(0);
}

// ========================================================================= //

void safeSend(int sockfd, const void *buf, size_t len, int flags){
    if (send(sockfd, buf, len, flags) != len) {
        Die("Mismatch in number of sent bytes");
    }
}
void safeRecv(int sockfd, void *buf, size_t len, int flags){
    int received;
    if ((received = recv(sockfd, buf, len, flags)) != len) {
        debug_print("Received: %2d bytes, should be: %d\n", received, (int)len);
        Die("Failed to receive bytes from server");
    }
    debug_print("Received: %2d bytes\n", received);
    
}
char* allocPack(int size){
    // Init pack with 0 byte
    char* pack = (char*)calloc(1, size);
    // If did not allocate memory
    if( pack == NULL ){
        Die("Error: Could not allocate memory");
    }
    return pack;
}

// JoinGame and return playerId
// return -1 if error
int joinGame(int sock){

    

    int packSize;
    char* pack;

    debug_print("%s\n", "Getting player name...");
    // Ask for player name if not predefined 
    if(playerName == NULL || strlen(playerName) < 1){
        fscanf(stdin, "%s", playerName);
    }


    packSize = 1 + MAXNICKSIZE;
    pack = allocPack(packSize);
    pack[0] = 0;
    printf("Size of name: %d\n", (int)sizeof(playerName));
    memcpy(&pack[1], playerName, MAXNICKSIZE);

    // Send JOIN packet
    debug_print("%s\n", "Sending JOIN packet...");
    pack[packSize-1]='\0';
    safeSend(sock, pack, packSize, 0);

    // Receive ACK
    packSize = 5;
    pack = allocPack(packSize);
    safeRecv(sock, pack, packSize, 0);
    debug_print("%s\n", "ACK reveived.");

    // ACK packet received
    if(pack[0] == 1){
        int id = pack[1]; 
        debug_print("Received Id: %d\n", id);
        if(id > 0){
            return (int)pack[1];
        }
        // Check for error type
        if( id == -1){
            printf("%s\n", "Could not join Server, reason: \
                                Player name is already taken");
            return -1;
        }
        if( id == -2){
            printf("%s\n", "Could not join Server, \
                                reason: Server is full");
            return -1;
        }
        if( id == -3){
            printf("%s\n", "Could not join Server, \
                                reason: Unknown");
            return -1;
        }
    }else{
        //TODO: Do something.
        return -1;
    }
}


// ========================================================================= //

int** allocateGameMap(int width, int height){
    int** map;
    map = (int**)malloc(sizeof(int)*height);
    // If did not allocate memory
    if( map == NULL ){
        printf("%s\n", "Error: Could not allocate memory");
        exit(1);
    }
    for (int i = 0; i < height; i++){
        map[i] = malloc(sizeof *map[i] * width);
        if( map[i] == NULL ){
            printf("%s\n", "Error: Could not allocate memory");
            exit(1);
        }
    }
    return map;
}

// ========================================================================= //

object_t** allocateGameMap2(int width, int height){
    object_t** map;
    map = (object_t**)malloc(sizeof(object_t)*height);
    // If did not allocate memory
    if( map == NULL ){
        printf("%s\n", "Error: Could not allocate memory");
        exit(1);
    }
    for (int i = 0; i < height; i++){
        map[i] = malloc(sizeof *map[i] * width);
        if( map[i] == NULL ){
            printf("%s\n", "Error: Could not allocate memory");
            exit(1);
        }
    }
    return map;
}

// ========================================================================= //

void printMappac(char* mappac){
    for (int i = 0; i < MAPHEIGHT*MAPWIDTH; ++i){
        printf("%3d", *((int*)mappac+i) );
        if((i+1) % MAPWIDTH == 0){
            printf("\n");
        }        
    }
}

// ========================================================================= //

void printMap(){
    for (int i = 0; i < MAPHEIGHT; ++i){
        for (int j = 0; j < MAPWIDTH; ++j){
           printf(" %s", translateType(MAP2[i][j].type));
        }
        printf("\n");
    }
};

char* translateType(int type){
    if (type == '0'){
        return "~";
    }
    if (type == '1'){
        return "=";
    }
    if (type == '2'){
        return ANSI_COLOR_RED "@" ANSI_COLOR_RESET;
    }
    if (type == '3'){
        return ".";
    }
    return "E"; // E for Error
}


// ========================================================================= //

void convertMappacToArray(char* mappac){
    MAP = allocateGameMap(MAPWIDTH, MAPHEIGHT);

    int row = 0;
    int col = 0;
    for (int i = 0; i < MAPHEIGHT*MAPWIDTH; ++i){
        // printf("%d\n", *((int*)mappac+i));
        // printf("row = %d, i = %d\n", row, i);
        MAP[row][col] = *((int*)mappac+i);
        col++;
        if((i+1) % MAPWIDTH == 0){
            row += 1;
            col = 0;
        }        
    }
    // printf("\n");
}

// ========================================================================= //

void convertMappacToArray2(char* mappac){
    MAP2 = allocateGameMap2(MAPWIDTH, MAPHEIGHT);

    int row = 0;
    int col = 0;
    for (int i = 0; i < MAPHEIGHT*MAPWIDTH; ++i){
        // printf("%d\n", *((int*)mappac+i));
        // printf("row = %d, i = %d\n", row, i);
        MAP[row][col] = *((int*)mappac+i);
        col++;
        if((i+1) % MAPWIDTH == 0){
            row += 1;
            col = 0;
        }        
    }
    // printf("\n");
}

// ========================================================================= //

void initializeMap(){
    MAP2 = allocateGameMap2(MAPWIDTH, MAPHEIGHT);
    for (int i = 0; i < MAPHEIGHT; ++i){
        for (int j = 0; j < MAPWIDTH; ++j){
            MAP2[i][j].type = '0';
            MAP2[i][j].id = -1;
            MAP2[i][j].x = j;
            MAP2[i][j].y = i;
            MAP2[i][j].status = 0;
        }
        printf("\n");
    }
}

// ========================================================================= //

void updateMap(object_t update){
    // Ceck if the update x and y are not outside border
    // If it is - fix it!
    if(update.x > MAPWIDTH){
        update.x = (int)update.x % MAPWIDTH;
    }
    if(update.y > MAPHEIGHT){
        update.y = (int)update.y % MAPHEIGHT;
    }
    // Check for existing objects
    object_t* exists = 0;
    for (int i = 0; i < MAPHEIGHT; ++i){
        for (int j = 0; j < MAPWIDTH; ++j){
            if (MAP2[i][j].id == update.id){
                exists = &MAP2[i][j];                
            }
        }
    }
    // If Object exits
    if(exists){
        //Do something
        debug_print("%s\n", "OBJECT ALREADY EXISTS!!");
    }else{
        int x = (int)update.x;
        int y = (int)update.y;
        
        debug_print("%s\n", "Before Update: ");
        printObj(MAP2[x][y]);
        MAP2[x][y].type = update.type;
        MAP2[x][y].id = update.id;
        MAP2[x][y].x = update.x;
        MAP2[x][y].y = update.y;
        MAP2[x][y].status = update.status;
        
        debug_print("%s\n", "After Update: ");
        printObj(MAP2[x][y]);
    }
}




// debug_print("Received Map MAP: %d\n", map[0]);
// debug_print("Received Map MAP: %d\n", map[4]);
// debug_print("Received Map MAP: %d\n", map[8]);
// debug_print("Received Map MAP: %d\n", map[12]);
// debug_print("Received Map MAP: %d\n", map[16]);
// debug_print("Received Map MAP: %d\n", map[20]);
// debug_print("Received Map MAP: %d\n", map[24]);
// debug_print("Received Map MAP: %d\n", map[28]);
// debug_print("Received Map MAP: %d\n", map[32]);