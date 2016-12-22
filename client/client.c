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


// Globals
char **MAP;
int mapWidth;
int mapHeight;
int playerId;
char playerName[MAX_NICK_SIZE+1] = "pacMonster007----END";
pthread_t  tid;   // Second thread


// ========================================================================= //

int joinGame(int sock);
char receivePacktype(int sock);
void waitForStart(int sock);
void* actionTherad(void *parm);   /* Thread function to client actions */

// ========================================================================= //

void* actionTherad(void *parm){
    int sock = (int)(intptr_t)parm;
    printf("%s\n", "a: <- left, w: up, d: -> right, s: down");
    char* pack;
    pack = allocPack(PSIZE_MOVE);
    pack[0] = PTYPE_MOVE;
    debug_print("Setting Id: %d\n", playerId);
    memcpy(&pack[1], &playerId, sizeof(playerId));
    debug_print("Id was set: %d\n", *(int*)(pack+1));
    while(1){
        char* move;
        fscanf(stdin,"%s", move);
        // Get vector
        if(move[0] == 'a'){
            pack[5] = DIR_LEFT;
        }else if( move[0] == 'w' ){
            pack[5] = DIR_UP;
        }else if( move[0] == 'd' ){
            pack[5] = DIR_RIGHT;
        }else if( move[0] == 's' ){
            pack[5] = DIR_DOWN;
        }else{
            continue;
        }
        /* Send move */
        safeSend(sock, pack, PSIZE_MOVE, 0);
        printf("You'r move was: %c\n", move[0]);
    }
    if(pack!=0){
        free(pack);
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
    // debug_print("Id Has been received and set: %d\n", playerId);
    waitForStart(sock);

       
    //Receive stuff from a server
    pthread_create(&tid, NULL, actionTherad, (void*)(intptr_t) sock);   
    
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
            int xfloor, yfloor; // Lai atrastu lauciņu uz kartes

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
                xfloor = (int)floorf(x);
                yfloor = (int)floorf(y);

                // Spēlētāju tipi pārklājas ar mapes objektu tipiem,
                // piešķiru vēl neizmantotas tipu vērtības
                debug_print("Setting %d type Player at x: %d y: %d\n",(int)*(playerObjBuffer+13), (int)x, (int)y);
                if((int)*(playerObjBuffer+13) == PLTYPE_PACMAN){
                    mapBuffer[yfloor][xfloor] = -1;
                }
                if((int)*(playerObjBuffer+13) == PLTYPE_GHOST){
                    mapBuffer[yfloor][xfloor] = -2;
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
            return id;
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
