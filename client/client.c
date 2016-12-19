#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <math.h>
#include <pthread.h>

// Shared functions for client and server
#include "../shared/shared.h"

//Unused
object_t **MAP2;
int MAPWIDTH;
int MAPHEIGHT;

// Globals
char **MAP;
int mapWidth;
int mapHeight;
int playerId;
char playerName[MAX_NICK_SIZE+1] = "pacMonster007----END";
pthread_t  tid;   // Second thread
//tcb- theard control block.;


// ========================================================================= //
// REDONE
int joinGame(int sock);
char receivePacktype(int sock);
void waitForStart(int sock);
void updateMap();

// TO BE REDONE
// object_t** allocateGameMap2(int width, int height);
// void initializeMap();

void* actionTherad();   /* Thread function to client actions */

// ========================================================================= //

void printObj(object_t obj){
    printf("ID: %1d Type: %c X: %2f Y: %2f ST: %d\n",\
        obj.id, obj.type, obj.x, obj.y, obj.state);
}

void* actionTherad(void *parm){
    int sock = (int)(intptr_t)parm;
    printf("%s\n", "a: <- left, w: up, d: -> right, s: down");
    while(1){
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


int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in gameserver;
    char packtype;

    char* pack;
    int packSize;

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
    
    playerId = joinGame(sock);
    waitForStart(sock);

       
    //Receive stuff from a server
    //pthread_create(&tid, NULL, actionTherad, (void*)(intptr_t) sock);   
    
    while(1){
        packtype = receivePacktype(sock);
        // Receive MAP
        if( (int)packtype == PTYPE_MAP ){
            debug_print("%s\n", "Getting MAP pack...");

            packSize = mapHeight*mapWidth;
            safeRecv(sock, *MAP, packSize, 0);
            printMap(MAP, mapWidth, mapHeight);
        }
        // Saņem PLAYERS paketi
        if( (int)packtype == PTYPE_PLAYERS ){
            // Kartes renderēšana
            float x,y;
            char** mapBuffer = allocateGameMap(mapWidth,mapHeight);
            memcpy(*mapBuffer,*MAP,mapWidth*mapHeight);

            debug_print("%s\n", "Getting PLAYERS pack...");
            // Saņem spēlētāju daudzumu
            int playerCount;
            safeRecv(sock, &playerCount, sizeof(playerCount), 0);
            debug_print("Receiving %d players...\n", playerCount);

            char playerObjBuffer[OSIZE_PLAYER];
            for (int i = 0; i < playerCount; ++i){
                memset(&playerObjBuffer,0,sizeof(playerObjBuffer));
                safeRecv(sock, &playerObjBuffer, OSIZE_PLAYER, 0);

                // Spēlētāja kordinātes
                x = *(float*)(playerObjBuffer+4);
                y = *(float*)(playerObjBuffer+8);
                // y = (float)playerObjBuffer[8];

                // Priekš pagaidu renderēšanas
                // BUG: mēģinot izmanto floor dabonu:
                // undefined reference to `floorf', lai gan iekļāvu
                // -lm kompilatora karodziņu.
                // int xfloor, yfloor;
                // xfloor = (int)floorf(x);
                // yfloor = (int)floorf(y);

                // Spēlētāju tipi pārklājas ar mapes objektu tipiem,
                // piešķiru vēl neizmantotas tipu vērtības
                debug_print("Setting %d type Player at x: %d y: %d\n",(int)*(playerObjBuffer+13), (int)x, (int)y);
                if((int)*(playerObjBuffer+13) == PLTYPE_PACMAN){
                    mapBuffer[(int)x][(int)y] = -1;
                }
                if((int)*(playerObjBuffer+13) == PLTYPE_GHOST){
                    mapBuffer[(int)x][(int)y] = -2;
                }
                debug_print("Received player with id: %d\n", *(int*)playerObjBuffer);
            }
            printMap(mapBuffer, mapWidth, mapHeight);
            free(mapBuffer); // TODO: Pārbaudīt vai visa atmiņa tiek atbrīvota
        }
   
    }

    fprintf(stdout, "\n");
   	close(sock);
   	exit(0);
}

// ========================================================================= //

// JoinGame and return playerId
// return -1 if error
int joinGame(int sock){

    char* pack;
    int id;

    debug_print("%s\n", "Getting player name...");
    // Ask for player name if not predefined 
    if(playerName == NULL || strlen(playerName) < 1){
        fscanf(stdin, "%s", playerName);
    }

    // 1 baits paketes tipam
    pack = allocPack(PSIZE_JOIN);
    pack[0] = PTYPE_JOIN;
    debug_print("Size of name: %d\n", (int)sizeof(playerName));
    memcpy(&pack[1], playerName, strlen(playerName));

    // Send JOIN packet
    debug_print("%s\n", "Sending JOIN packet...");
    safeSend(sock, pack, PSIZE_JOIN, 0);
    free(pack);

    // Receive ACK
    pack = allocPack(PSIZE_ACK);
    safeRecv(sock, pack, PSIZE_ACK, 0);
    debug_print("%s\n", "ACK reveived.");

    if(pack[0] == PTYPE_ACK){
        int id = (int)pack[1];
        free(pack);
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
        free(pack);
        return -1;
    }
}

// ========================================================================= //

void waitForStart(int sock){
    char packtype;
    char *pack;

    packtype = receivePacktype(sock);
    if((int)packtype == PTYPE_START){
        pack = allocPack(PSIZE_START-1);
        safeRecv(sock, pack, PSIZE_START-1, 0);
        // Set map sizes (globals)
        mapHeight = (int)pack[0];
        mapWidth = (int)pack[1];
        debug_print("Received Map sizes, width: %2d  height: %2d plx: %d ply: %d\n",\
                        mapWidth, mapHeight, (int)pack[2], (int)pack[3] );
        MAP = allocateGameMap(mapWidth, mapHeight);
        // TODO: Get player chords
        // For now I have no idea where to
        // put them ...
    }else{
        // TODO: Do something
    }

    // Free memory
    if(pack != 0){
        free(pack);
    }
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

void initializeMap(){
    MAP2 = allocateGameMap2(MAPWIDTH, MAPHEIGHT);
    for (int i = 0; i < MAPHEIGHT; ++i){
        for (int j = 0; j < MAPWIDTH; ++j){
            MAP2[i][j].type = '0';
            MAP2[i][j].id = -1;
            MAP2[i][j].x = j;
            MAP2[i][j].y = i;
            MAP2[i][j].state = 0;
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
        MAP2[x][y].state = update.state;
        
        debug_print("%s\n", "After Update: ");
        printObj(MAP2[x][y]);
    }
}

// ========================================================================= //
