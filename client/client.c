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
    int sockTCP, sockUDP;
    struct sockaddr_in gameserver, gameclient;
    socklen_t clientAdrrLength;
    char packtype;
    char pack[1024];

    //char* pack;
    int packSize;

    if (argc != 3) {
      fprintf(stderr, "USAGE: client <server_ip> <port>\n");
      exit(1);
    }

    // ------- Izveidojam TCP un UDP soketus ------------------------------- //
    if ((sockTCP = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
      Die(ERR_SOCKET);
    }
    if ((sockUDP = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
      Die(ERR_SOCKET);
    }

    // ------- Izveidojam servera adresi ----------------------------------- //
    memset(&gameserver, 0, sizeof(gameserver));       /* Clear struct */
    gameserver.sin_family = AF_INET;                  /* Internet/IP */
    gameserver.sin_addr.s_addr = inet_addr(argv[1]);  /* IP address */
    gameserver.sin_port = htons(atoi(argv[2]));       /* server port */

    
    // ----------- Savienojas ar serveri - TCP ----------------------------- //
    if (connect(sockTCP,(struct sockaddr *) &gameserver,
                sizeof(gameserver)) < 0) {
      Die(ERR_CONNECT);
    }

    // ------- Bindojam UDP soketu pie TCP adreses ------------------------- //
    clientAdrrLength = sizeof(gameclient);
    getsockname(sockTCP, (struct sockaddr*)&gameclient, &clientAdrrLength);
    if (bind(sockUDP,(struct sockaddr *)&gameclient,clientAdrrLength)<0){
        Die(ERR_BIND);        
    } 

    // ----------- Savienojas ar serveri - UDP ----------------------------- //
    if (connect(sockUDP,(struct sockaddr *) &gameserver,
                sizeof(gameserver)) < 0) {
      Die(ERR_CONNECT);
    }

    playerId = joinGame(sockTCP);
    waitForStart(sockTCP);

    // Jauns pavediens UDP pakešu sūtīšanai
    pthread_create(&tid, NULL, actionTherad, (void*)(intptr_t) sockUDP);   
    
    while(1){
        memset(&pack, 0, 1024);
        safeRecv(sockUDP,&pack,sizeof(pack),0);
        packtype = pack[0];
        // Receive MAP
        if( (int)packtype == PTYPE_MAP ){
            debug_print("%s\n", "Getting MAP pack...");

            packSize = mapHeight*mapWidth;
            memcpy(*MAP, &pack[1], packSize);
            // safeRecv(sockUDP, *MAP, packSize, 0);
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
            int32_t playerCount;
            memcpy(&playerCount, &pack[1],sizeof(playerCount));
            debug_print("Receiving %d players...\n", playerCount);

            char playerObjBuffer[OSIZE_PLAYER];
            char* current = &pack[5];
            for (int i = 0; i < playerCount; ++i,current += 14){
                memset(&playerObjBuffer,0,sizeof(playerObjBuffer));
                memcpy(&playerObjBuffer,current, OSIZE_PLAYER);

                // Spēlētāja kordinātes
                x = *(float*)(playerObjBuffer+4);
                y = *(float*)(playerObjBuffer+8);

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
    close(sockTCP);
   	close(sockUDP);
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
        int32_t id = batoi(&pack[1]);
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
