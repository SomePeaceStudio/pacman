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

//TODO: saņemt END paketi
//TODO: saņemt START paketi ik reizes, kad beidzas mačš

// Globals
char **MAP;
int mapWidth;
int mapHeight;
int32_t playerId;
char playerName[MAX_NICK_SIZE+1] = "pacMonster007----END";
pthread_t  tid;   // Second thread
int sockTCP, sockUDP; // Lai visi pavedieni var viegli piekļūt klāt
FILE* chatFile;

// ========================================================================= //

int joinGame(int sock);
char receivePacktype(int sock);
void waitForStart(int sock);
void* actionTherad(void *parm);   /* Thread function to client actions */
int sendQuit(int sock);
void readMessage(int sock);
int readTCPPacket(int sock);
void readStart(int sock);

// ========================================================================= //

void* actionTherad(void *parm){
    //int sock = (int)(intptr_t)parm;
    printf("%s\n", "a: <- left, w: up, d: -> right, s: down");
    char* pack;
    char move;
    int32_t messageSize;

    while(1){
        char command[MAX_MESSAGE_SIZE];
        // bzero(command, MAX_MESSAGE_SIZE);
        fgets(command, MAX_MESSAGE_SIZE, stdin);
        // command[MAX_MESSAGE_SIZE-1]='\0';

        if(strlen(command)>3 && command[0]=='/'){
            if(command[1]=='m'){
                messageSize = strlen(&command[3]); 
                int packSize = 1+4+4+messageSize;

                char* pack = allocPack(packSize);

                pack[0] = PTYPE_MESSAGE;               //0. baits
                itoba(playerId, &pack[1]);              //1. - 4. baits
                itoba(messageSize, &pack[5]);              //5. - 8. baits
                strncpy(&pack[9], &command[3], messageSize); //9. - x. baits
                safeSend(sockTCP, pack, packSize, 0);
                free (pack);
                continue;
            }
            continue;
        }

        // Pārtulko par virzienu
        if(command[0] == 'a'){
            move = DIR_LEFT;
        }else if( command[0] == 'w' ){
            move = DIR_UP;
        }else if( command[0] == 'd' ){
            move = DIR_RIGHT;
        }else if( command[0] == 's' ){
            move = DIR_DOWN;
        }else{
            continue;
        }
        /* Send move */

        pack = allocPack(PSIZE_MOVE);

        // debug_print("Setting Id: %d\n", playerId);
        pack[0] = PTYPE_MOVE;
        itoba(playerId, &pack[1]);
        // debug_print("Id was set: %d\n", batoi(&pack[1]));
        pack[5] = move;
        safeSend(sockUDP, pack, PSIZE_MOVE, 0);

        printf("You'r move was: %c\n", command[0]);
        free(pack);
    }
    if(pack!=0){
        free(pack);
    }
};

int main(int argc, char *argv[]) {
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

    // Atver čata failu
    if((chatFile = fopen("chat", "w")) == NULL){
        Die("Could not open chat file");    
    }   

    playerId = joinGame(sockTCP);
    waitForStart(sockTCP);

    // Jauns pavediens UDP pakešu sūtīšanai
    pthread_create(&tid, NULL, actionTherad, (void*)(intptr_t) sockUDP);   
    
    while(1){
        readTCPPacket(sockTCP);

        memset(&pack, 0, 1024);
        safeRecv(sockUDP,&pack,sizeof(pack),0);
        
        // Receive MAP
        if( (int)pack[0] == PTYPE_MAP ){
            debug_print("%s\n", "Getting MAP pack...");

            packSize = mapHeight*mapWidth;
            memcpy(*MAP, &pack[1], packSize);
            // safeRecv(sockUDP, *MAP, packSize, 0);
            printMap(MAP, mapWidth, mapHeight);
            continue;
        }
        // Saņem PLAYERS paketi
        if( (int)pack[0] == PTYPE_PLAYERS ){
            // Kartes renderēšana
            float x,y;
            int xfloor, yfloor; // Lai atrastu lauciņu uz kartes

            char** mapBuffer = allocateGameMap(mapWidth,mapHeight);
            memcpy(*mapBuffer,*MAP,mapWidth*mapHeight);

            debug_print("%s\n", "Getting PLAYERS pack...");
            // Saņem spēlētāju daudzumu
            int32_t playerCount = batoi(&pack[1]);
            debug_print("Receiving %d players...\n", playerCount);

            char playerObjBuffer[OSIZE_PLAYER];
            char* current = &pack[5];
            for (int i = 0; i < playerCount; ++i,current += 14){
                memset(&playerObjBuffer,0,sizeof(playerObjBuffer));
                memcpy(&playerObjBuffer,current, OSIZE_PLAYER);

                // Spēlētāja kordinātes
                x = batof(playerObjBuffer+4);
                y = batof(playerObjBuffer+8);

                // Priekš pagaidu renderēšanas
                xfloor = (int)floorf(x);
                yfloor = (int)floorf(y);

                // Mirušus nerāda
                if(playerObjBuffer[12]==PLSTATE_DEAD){
                    continue;
                }
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
            continue;
        }
        if( (int)pack[0] == PTYPE_SCORE ){
            for(int i = 0; i < batoi(&pack[1]); i++){
                printf("Id: %d Score: %d\n",batoi(&pack[5+i*(INT_SIZE*2)]),\
                    batoi(&pack[9+i*(INT_SIZE*2)]));
                
            }
            continue;
        }
        
    }

    fprintf(stdout, "\n");
    close(sockTCP);
   	close(sockUDP);
    // Aizver čata failu
    if(fclose(chatFile) != 0){
        printf("%s\n", "Could not close chatFile");
    }
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
    packtype = receivePacktype(sock);
    if((int)packtype == PTYPE_START){
        readStart(sock);
        // TODO: Get player chords
        // For now I have no idea where to
        // put them ...
    }else{
        // TODO: Do something
    }
}

// ========================================================================= //

int sendQuit(int sock){
    char pack[PSIZE_QUIT];
    pack[0] = PTYPE_QUIT;
    itoba(playerId, &pack[1]);
    if(safeSend(sock, &pack, PSIZE_QUIT, 0) < 0){
        return 1;
    };
    debug_print("%s\n", "Quit was sent!");
    return 0;
}

// ========================================================================= //

int readTCPPacket(int sock){
    debug_print("%s\n", "Receiving TCP packets.. ");
    char packtype;
    if(recv(sock, &packtype, 1, MSG_DONTWAIT) < 0){
        return 1;
    }
    if(packtype == PTYPE_MESSAGE){
        debug_print("%s\n", "Receiving Chat message.. ");
        readMessage(sock);
        return 0;
    }
    if(packtype == PTYPE_END){
        debug_print("%s\n", "Receiving END. ");
        return 0;
    }
    if(packtype == PTYPE_START){
        debug_print("%s\n", "Receiving START. ");
        readStart(sock);
        return 0;
    }
    debug_print("Received <UNKNOWN> TCP packtype: %d playerID: %d\n", (int)packtype, playerId);
    return 1;
}
void readMessage(int sock){
    char header[8];
    int32_t messageSize;
    int32_t messageId; // Spēlētāja id ziņas paketē
    char* message;

    serverRecv(sock, &header, 8, 0);

    messageId = batoi(header);
    messageSize = batoi(&header[4]);

    message = allocPack(messageSize+1);
    serverRecv(sock, message, messageSize, 0);
    message[messageSize] = '\0';

    printf("MESSAGE: %s, from client: %d\n",message, messageId);
    
    
    debug_print("%s\n", "Reading Map File...");
    if(fprintf(chatFile, "[From: id-%d] %s\n",messageId, message) == 0){
        Die("Could not read map's widht and height");
    }
    fflush(chatFile);
    free (message);
}
void readStart(int sock){
    if(MAP != 0){
        free(MAP);
    }
    char pack[PSIZE_START-1];
    safeRecv(sock, pack, PSIZE_START-1, 0);
    // Set map sizes (globals)
    mapHeight = (int)pack[0];
    mapWidth = (int)pack[1];
    debug_print("Received Map sizes, width: %2d  height: %2d plx: %d ply: %d\n",\
                    mapWidth, mapHeight, (int)pack[2], (int)pack[3] );
    MAP = allocateGameMap(mapWidth, mapHeight);
}
