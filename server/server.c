#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <math.h>
#include <sys/ioctl.h>
#include <stdbool.h>

// Shared functions for client and server
#include "../shared/shared.h"
#include "../shared/threads.h"

#define MAXPENDING 5    /* Max connection requests */

// ------------------------------------------------------------------------- //

typedef struct objectNode{
    object_t object;
    struct objectNode *next;
} objectNode_t;

// ------------------------------------------------------------------------- //

int32_t ID = 1;
objectNode_t *STATE;

char* mapFileName;
char** MAP;
int MAPHEIGHT;
int MAPWIDTH;
volatile int playerCount = 0;   // Pašreizējais spēlētāju skaits
volatile short END = 1;         // Nosaka vai spēle ir beigusies
volatile int randSeed = 1;      // seed priekš rand; 
pthread_t  mainGameThread;      // Galvenais GAME-LOOP
thread_pool_t threadPool;  

// ========================================================================= //

int getId() { return ID++; }
void addObjectNode(objectNode_t **start, objectNode_t *newNode);
void addObjectNodeEnd(objectNode_t **start, objectNode_t *newNode);
objectNode_t *createObjectNode(object_t* gameObj);
void deleteObjectWithId(objectNode_t **start, int id);
int deleteObjectNode(objectNode_t **start, objectNode_t *node);
void printPlayerList(objectNode_t *start);
object_t* getPlayer(objectNode_t *start, int id);

void sendStateUpdate(int sock);
void sendMapUpdate(int sock);
void sendPlayersState(int sock);
void sendStart(int sock, int32_t playerId);
void sendScores(int sock);
void sendJoined(int32_t playerId, char name[20]);
void sendPlayerDisconnected(int32_t playerId);
//Nosūta paketi visiem spēlētājiem. usTCP nosaka TCP/UDP
void sendBroadcast(char* pack, size_t length, bool useTcp);
int sendEnd(int sock);
int handleJoin(int sock, int32_t playerId);
int readTCPPacket(int sock, int32_t playerId);
int readQuit(int sock, int32_t playerId);
void readMessage(int sock, int32_t playerId);


void getNewMap(char* filename);
void initNewPlayer(int id, int type, char *name);
int getPlayerType();
void setSpawnPoint(float *x, float *y);
void updateState();
void resetGame();
void resetPlayers();
void setPlayerDisconnected(int32_t playerId);  

int isGameEnd();
int isPointsLeft();
int isAnyPacmanLive();

void* actionThread(void *parm);   // Funkcija priekš klienta gājienu apstrādes
void* handleClient(void *parm);
void* mainGameLoop();


// ========================================================================= //

void* mainGameLoop(){
    END = 0;
    while(1){
        // Pavirza spēlētājus uz priekšu
        updateState();
        // usleep(250000); //250 milisekundes
        sleep(1);

        // Nosaka vai ir spēles beigas 
        END = isGameEnd();
        while(END){
            while(playerCount == 0){}
            debug_print("%s\n", "Reseting Game!");
            resetGame();
            debug_print("%s\n", "New game in: 3s");
            sleep(1);
            debug_print("%s\n", "New game in: 2s");
            sleep(1);
            debug_print("%s\n", "New game in: 1s");
            sleep(1);
            END = 0;
        }
    }

}

// ========================================================================= //

void* actionThread(void *parm){
    int sock = ((action_thread_para_t*)parm)->socket;
    int32_t playerId = ((action_thread_para_t*)parm)->id;
    char pack[1024];
    object_t *player;
    while(1){
        memset(&pack, 0, 1024);

        if(serverRecv(sock, &pack, sizeof(pack), 0) < 0){
            // Ja neizdevās nolasīt datus no soketa,
            // tiek pieņemts, ka spēlētājs ir atvienojies
            setPlayerDisconnected(playerId);
            return 0;
        };

        if((int)pack[0] == PTYPE_MOVE){
            int32_t id = batoi(&pack[1]);
            char mdir = *(pack+5);
            debug_print("%s\n", "Done receiving!");
            debug_print("Id: %d\n", id);
            debug_print("mdir: %d\n", mdir);

            player = getPlayer(STATE, id);
            if(player!=0){
                if(player->state == PLSTATE_DEAD){
                    debug_print("Dead meat, id: %d don't move!\n" , id);
                    continue;
                }
                player->mdir = mdir;
                continue;
            }
            debug_print("Could not find player to move, id: %d\n" , id);
        }else{
            debug_print("Received unkown type packet in UDP loop: %d\n", pack[0]);
            continue;
        }
    }
    debug_print("%s\n", "Exiting actionThread...");
};

void* handleClient(void *sockets) {
    sockets_t* socketStruct = (sockets_t*)sockets;
    
    int sockTCP = socketStruct->tcp;   // TCP sokets priekš 
                                       // JOIN,ACK,START,END un QUIT paketēm
    int sockUDP = socketStruct->udp;   // UDP sokets visu pārējo pakešu apstrādei
    pthread_t* actionThreadId;      // Pavediens Spēlētāja sūtītām UDP pakešu apstrādei
    int32_t playerId = getId();     // Spēlētāja ID
    object_t* player;               // Norāde uz spēlētāju, spēlētāju sarakstā
    char playerName[MAX_NICK_SIZE+1];
 
    // Izveidoju jaunu pavedienu priekš spēlētāja UDP
    // sūtīto pakešu apstrādei
    actionThreadId = getFreeThread(&threadPool);
    action_thread_para_t para = {sockUDP,playerId}; 
    pthread_create(actionThreadId, NULL, actionThread, (void*)&para); 

    //------- JOIN / ACK -------//
    if(handleJoin(sockTCP, playerId)!=0){
        // Ja neizdevās saņemt pirmo paketi JOIN,
        // tad savienojums tiek pārtraukts
        close(sockTCP);
        close(sockUDP);
        freeThread(&threadPool,*actionThreadId);
        freeThread(&threadPool,pthread_self());
        return 0;
    };

    player = getPlayer(STATE,playerId);
    player->sockets = *socketStruct;
    
    //------- START -------//
    sendStart(sockTCP, playerId);
   

    // Main GAME loop
    while(1){

        // Nolasa TCP paketi, ja nolasīta ir QUIT, tad atgriež 2
        if(readTCPPacket(sockTCP, playerId) == 2){
            break;
        };

        // Pārbauda vai spēlētājs ir atvienojies
        if(player->disconnected){
            break;
        }
        // Pārbauda vai spēle ir beigusies
        if(END){
            // Nosūta END paketi
            sendEnd(sockTCP);
            while(END){}
            // Atkārtoti sūta start, lai klients var inicializēt karti
            sendStart(sockTCP, playerId);
        }

        // Sūta MAP paketi
        sendMapUpdate(sockUDP);

        // Sūta PLAYERS paketi
        sendPlayersState(sockUDP);

        // Sūta Spēlētāju punktus
        sendScores(sockUDP);

        sleep(1);
        // usleep(250000); //250 milisekundes
    }

    debug_print("%s\n", "Client Disconnected, closing threads...");
    deleteObjectWithId(&STATE, playerId);
    sendPlayerDisconnected(playerId);
    close(sockTCP);
    close(sockUDP);
    freeThread(&threadPool,*actionThreadId);
    freeThread(&threadPool,pthread_self());
    return 0;
}


int main(int argc, char *argv[]) {
    int serversock;
    sockets_t clientSock;
    struct sockaddr_in gameserver, gameclient;
    

    if (argc != 3) {
      fprintf(stderr, "USAGE: server <port> <mapfile>\n");
      exit(1);
    }
    // --------- Izveidojam TCP soketu ------------------------------------- //
    if ((serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      Die("Failed to create socket");
    }
    
    // --------- Izveidojam servera adreses struktūru ---------------------- //
    memset(&gameserver, 0, sizeof(gameserver));       /* Clear struct */
    gameserver.sin_family = AF_INET;                  /* Internet/IP */
    gameserver.sin_addr.s_addr = htonl(INADDR_ANY);   /* Incoming addr */
    gameserver.sin_port = htons(atoi(argv[1]));       /* server port */

    // --------- Bindojam TCP soketus pie servera adreses ------------------ //
    if (bind(serversock, (struct sockaddr *) &gameserver,
                               sizeof(gameserver)) < 0) {
        Die("Failed to bind the server socket");
    }

    // ---------------------- Inicializē Karti ----------------------------- //

    mapFileName = malloc(strlen(argv[2] + 1));
    strcpy(mapFileName, argv[2]);
    getNewMap(mapFileName);

    // --------------------------------------------------------------------- //

    // Inicializē pavedienu kopu, sāk spēles galveno pavedienu
    initThreadPool(&threadPool);
    pthread_create(&mainGameThread, NULL, mainGameLoop,0);


    /* Listen on the server socket */
    if (listen(serversock, MAXPENDING) < 0) {
        Die("Failed to listen on server socket");
    }

    /* Run until cancelled */
    while (1) {
        unsigned int clientlen = sizeof(gameclient);
        // Gaidām TCP savienojumu
        if ((clientSock.tcp =
           accept(serversock, (struct sockaddr *) &gameclient,
                  &clientlen)) < 0) {
            debug_print("%s\n", ERR_CONNECT);
            continue;            
        }
        fprintf(stdout, "Client connected: %s\n", 
                    inet_ntoa(gameclient.sin_addr));

        // Izveidojam UDP soketu
        if ((clientSock.udp = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            debug_print("%s\n", ERR_SOCKET);
            close(clientSock.tcp);
            continue;
        };

        int optval = 1;
        setsockopt(clientSock.udp, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        if (bind(clientSock.udp, (struct sockaddr *)&gameserver, sizeof(gameserver))<0){
            perror(ERR_BIND);
            close(clientSock.tcp);
            close(clientSock.udp);
            continue;
        };
        if (connect(clientSock.udp,(struct sockaddr *) &gameclient, clientlen) < 0) {
            debug_print("%s\n", ERR_CONNECT);
            close(clientSock.tcp);
            close(clientSock.udp);
            continue;
        }

        sockets_t tmp;
        memcpy(&tmp, &clientSock, sizeof(sockets_t));
        // Apstrādājam klientu atsevišķā pavedienā
        debug_print("Free Theads In Pool: %d\n", countFreeThreads(&threadPool));
        pthread_create( getFreeThread(&threadPool) , NULL, handleClient, (void*)&tmp);
    }
}

// ========================================================================= //

void updateState(){
    float *x, *y;
    int mdir;
    int canMove;            // Vai gājienu var izdarīt (vai priekšā nav siena)
    //FIXME: Mainot speed uz mazāku spēlētājs var ieiet sienā
    float speed = 1;        // Vina gājiena lielums vienā game-tick
    float xSpeed, ySpeed;   // Spēlētāja x un y ātrums;
    char *nextMapTile;      // Kartes lauciņš uz kuru spēlētājs pārvietosies
    float tileSize = 1;     // vienas rūtiņas platums
    // NOTE: tiem pagaidām ņemti vērā tikai spēlētāju kolīzijās, ja izmēri tiek
    // mainīti būs nepieciešams pārrakstīt Spēlētāju/Kartes obj. kolīzijas
    float pacmanWidth   = 0.9; // < 1 lai spawnojoties uzreiz nesaskartos  
    float pacmanHeight  = 0.9; 
    float ghostWidth    = 0.9;   
    float ghostHeight   = 0.9;  

    // Veicam visas kustības visiem spēlētājiem atkarībā no to kursības virziena
    // Apskatām visas kolīzijas ar kartes objektiem
    for(objectNode_t *current = STATE; current != 0; current = current->next){
        // Iegūstam koordinātes un kustības virzienu
        x = &(current->object.x);
        y = &(current->object.y);
        mdir = (int)(current->object.mdir);

        // Atšifrējam kustības virzienu kā ātrumu x/y virzienā
        xSpeed = 0; ySpeed = 0;
        if(mdir == DIR_UP){
            ySpeed = -speed;
        }else if( mdir == DIR_DOWN ){
            ySpeed = speed;
        }else if( mdir == DIR_LEFT ){
            xSpeed = -speed;
        }else if( mdir == DIR_RIGHT ){
            xSpeed = speed;
        }else if ( mdir == DIR_NONE){
            continue;
        }

        // Spēlētājs mēģina iziet ārpus kartes robežām
        if((int)floorf(*y+ySpeed+tileSize/2) >= MAPHEIGHT){
            *y+=ySpeed-MAPHEIGHT;
            continue; 
        }
        if((int)floorf(*y+ySpeed+tileSize/2) < 0){
            *y+=ySpeed+MAPHEIGHT;
            continue; 
        }
        if((int)floorf(*x+xSpeed+tileSize/2) >= MAPWIDTH){
            *x+=xSpeed-MAPWIDTH;
            continue;  
        }
        if((int)floorf(*x+xSpeed+tileSize/2) < 0){
            *x+=xSpeed+MAPWIDTH;
            continue;  
        }

        canMove = 1;
        nextMapTile = &MAP[(int)floorf(*y+ySpeed+tileSize/2)][(int)floorf(*x+xSpeed+tileSize/2)];
        // Kolīzijas
        if(current->object.type == PLTYPE_PACMAN || current->object.type == PLTYPE_GHOST){
            // Kolīzija ar sienu
            if((int)*nextMapTile == MTYPE_WALL){
                debug_print("%s\n", "Player collides with wall.");
                canMove = 0;
            }                  
        }
        if(current->object.type == PLTYPE_PACMAN){
            if((int)*nextMapTile == MTYPE_DOT){
                debug_print("%s\n", "Packman collides with Point.");
                *nextMapTile = MTYPE_EMPTY;
                current->object.points += 1;
            }
            if((int)*nextMapTile == MTYPE_POWER){
                debug_print("%s\n", "Packman collides with Powerup.");
                *nextMapTile = MTYPE_EMPTY;
                // TODO: Do something
            }
            if((int)*nextMapTile == MTYPE_INVINCIBILITY){
                debug_print("%s\n", "Packman collides with Invincibility.");
                *nextMapTile = MTYPE_EMPTY;
                // TODO: Do something
            } 
            if((int)*nextMapTile == MTYPE_SCORE){
                debug_print("%s\n", "Packman collides with Extra points.");
                *nextMapTile = MTYPE_EMPTY;
                current->object.points += 10;
            }                   
        }
        if(canMove){
            *x+=xSpeed;
            *y+=ySpeed; 
        }
    }
    // Pārbaudīt spēlētāju kolīzijas Pacman/Ghost
    float iWidth, iHeight, jWidth, jHeight;
    // x1 - kreisās malas viduspunkts
    // x2 - labās malas viduspunkts
    // y1 - augšējās malas viduspunkts
    // y2 - apakšējās malas viduspunkts
    float ix1,ix2,iy1,iy2,jx1,jx2,jy1,jy2;
    for(objectNode_t *i = STATE; i != 0; i = i->next){
        // Mirušiem spēlētājiem neveic aprēķinus
        if(i->object.state == PLSTATE_DEAD){
            continue;
        }
        // Iegūstas spēlētāja tipa izmērus
        if(i->object.type == PLTYPE_PACMAN){
            iWidth = pacmanWidth;
            iHeight = pacmanHeight;
        }else{
            iWidth = ghostWidth;
            iHeight = ghostHeight;
        }
        ix1 = i->object.x - iWidth/2;
        ix2 = i->object.x + iWidth/2;
        iy1 = i->object.y - iHeight/2;
        iy2 = i->object.y + iHeight/2;

        for(objectNode_t *j = STATE; j != 0; j = j->next){
            // Ja spēlētāju tipi ir vienādi - nekas nenotiek
            if(i->object.type == j->object.type || \
                i->object.id == j->object.id    || \
                j->object.state == PLSTATE_DEAD)
            {
                continue;
            }
            // Iegūstas spēlētāja tipa izmērus
            if(j->object.type == PLTYPE_PACMAN){

                jWidth = pacmanWidth;
                jHeight = pacmanHeight;
            }else{
                jWidth = ghostWidth;
                jHeight = ghostHeight;
            }
            jx1 = j->object.x - jWidth/2;
            jx2 = j->object.x + jWidth/2;
            jy1 = j->object.y - jHeight/2;
            jy2 = j->object.y + jHeight/2;
            // Pārbauda vai objekti pārklājas
            printf("\n\n%s\n", "Checking Collission");
            printf("%f<=%f && %f>=%f && %f<=%f && %f>=%f\n\n",\
                ix1,jx2,ix2,jx1,iy1,jy2,iy2,jy1);
            if(ix1<=jx2 && ix2>=jx1 && iy1<=jy2 && iy2>=jy1){
                // Pacman tiek apēsts
                if(i->object.type == PLTYPE_PACMAN){
                    i->object.state = PLSTATE_DEAD;
                    j->object.points +=1;
                }else{
                    j->object.state = PLSTATE_DEAD;
                    i->object.points +=1;
                }
            }
            
        
        }
    }
}

// ========================================================================= //

// Atgriež 1, ja ir notikusi ķļūda;
// Atgriež 0, ja izdevās pievienot klientu;
int handleJoin(int sock, int32_t playerId){
    int packSize;
    char pack[PSIZE_JOIN];
    if(serverRecv(sock, &pack, PSIZE_JOIN,0)<0){
        return 1;
    };
    char playerName[MAX_NICK_SIZE+1];

    if((int)pack[0] == PTYPE_JOIN){
        // Iegūst lietotājvārdu
        memcpy(&playerName, &pack[1],MAX_NICK_SIZE);
        playerName[MAX_NICK_SIZE] = '\0';
        debug_print("Received playerName: %s\n", playerName);

        //TODO: Uzlikt MUTEX, lai spēlētāju skaits nenojūk
        initNewPlayer(playerId, getPlayerType(), playerName);

        // Sūta ACK
        bzero(&pack, sizeof(pack));
        pack[0] = PTYPE_ACK;

        // Iekopē spēlētāja id baitus paketē
        itoba(playerId, &pack[1]);
        printf("About to send ACK, id: %d\n", batoi(&pack[1]));

        if(safeSend(sock, &pack, PSIZE_ACK, 0) < 0){
            debug_print("%s\n", "Could not send ACK packet");
            return 1;
        }
        
        //Sūta JOINED visiem pārējiem pārējiem
        debug_print("%s\n", "Sending JOINED packet");
        sendJoined(playerId, playerName);
    }else{
        debug_print("%s\n", "Did not receive JOIN packet");
        return 1;
    }
    return 0;
}

// ========================================================================= //

void sendStart(int sock, int32_t playerId){
    int packSize;
    char pack[PSIZE_START];

    debug_print("%s\n", "Sending START packet...");
    pack[0] = 2;
    pack[1] = MAPHEIGHT;
    pack[2] = MAPWIDTH;
    pack[3] = getPlayer(STATE, playerId)->x;
    pack[4] = getPlayer(STATE, playerId)->y;

    // Ja neizdodas nosūtīt kartes izmērus tiek pieņemts,
    // ka klients ir atvienojies
    if(safeSend(sock, pack, PSIZE_START, 0)<0){
        setPlayerDisconnected(playerId);
    }
}

// ========================================================================= //

void sendPlayersState(int sock){
    int packSize;
    char* pack;

    //1 baits paketes tipam, 4 - spēlētāju skaitam (int)
    packSize = 1 + 4 + playerCount*OSIZE_PLAYER;
    pack = allocPack(packSize);
    pack[0] = PTYPE_PLAYERS;
    itoba(playerCount, &pack[1]);

    char* currObj = &pack[5]; // Norāda uz sākumvietu, kur rakstīt objektu
    for(objectNode_t *current = STATE; current != 0; current = current->next){
        //Id
        itoba(current->object.id, currObj);
        currObj += 4;
        
        //X koordināta
        ftoba(current->object.x, currObj);
        currObj += 4;
        
        //Y koordināta
        ftoba(current->object.y, currObj);
        currObj += 4;
        
        //Ieraksta spēlētāja stāvokli
        *currObj = current->object.state;
        currObj += 1;
        
        //Ieraksta spēlētāja tipu
        *currObj = current->object.type;
        currObj += 1;
    }

    // Send MAP packet
    debug_print("Sending %d PLAYERS... packSize: %d\n", batoi(&pack[1]), packSize);
    printPlayerList(STATE);
    safeSend(sock, pack, packSize, 0);
    if(pack != 0){
        free(pack);
    }
}

// ========================================================================= //

void sendScores(int sock){
    char pack[DEFAULT_PACK_SIZE];
    //tips + skaits + (id un score katram spēlētājam)
    int packSize = ENUM_SIZE + INT_SIZE + playerCount*(INT_SIZE*2);
    if( DEFAULT_PACK_SIZE < packSize ){
        debug_print("Scores pack size is too large: %d max: %d\n",\
            packSize,DEFAULT_PACK_SIZE);
    }

    pack[0] = PTYPE_SCORE;
    itoba(playerCount, &pack[1]);

    char* currObj = &pack[5]; // Norāda uz sākumvietu, kur rakstīt objektu
    for(objectNode_t *current = STATE; current != 0; current = current->next){
        itoba(current->object.points, &currObj[0]);
        itoba(current->object.id, &currObj[4]);
        currObj += 8;
    }
    debug_print("%s\n", "Sending SCORE packet...");
    safeSend(sock, pack, packSize, 0);
}

// ========================================================================= //

void sendMapUpdate(int sock){
    int packSize;
    char* pack;

    packSize = 1 + MAPWIDTH * MAPHEIGHT;
    pack = allocPack(packSize);
    pack[0] = PTYPE_MAP;
    memcpy(&pack[1], *MAP, MAPWIDTH * MAPHEIGHT);
    
    printf("MAP packet:\n");
    
    printMappacPretty(&pack[1], MAPWIDTH, MAPHEIGHT);
    // Send MAP packet
    debug_print("%s\n", "Sending MAP packet...");
    safeSend(sock, pack, packSize, 0);
    if(pack!=0){
    free(pack);
    }
}

// ========================================================================= //

void sendJoined(int32_t playerId, char name[20]) {
    char* pack = allocPack(PSIZE_JOINED);
    pack[0] = PTYPE_JOINED;       //0. baits
    itoba(playerId, &pack[1]);    //1. - 4. baits
    strncpy(&pack[5], name, 20);  //5. - 24. baits
    sendBroadcast(pack, PSIZE_JOINED, true);
    free (pack);
}

// ========================================================================= //

void sendPlayerDisconnected(int32_t playerId) {
    debug_print("Sending PL-disconect pack, pl-id: %d\n", playerId);
    char* pack = allocPack(PSIZE_PLAYER_DISCONNECTED);
    pack[0] = PTYPE_PLAYER_DISCONNECTED;
    itoba(playerId, &pack[1]);
    sendBroadcast(pack, PSIZE_PLAYER_DISCONNECTED, true);
    free(pack);
}

// ========================================================================= //

void sendBroadcast(char* pack, size_t length, bool useTcp) {
    for (objectNode_t* current = STATE; current != NULL; current = current->next) {
        int sock = useTcp ? current->object.sockets.tcp : current->object.sockets.udp;
        if (sock != 0) {
            printf("%s\n", "SENDING!!!!!!! Broadcast!!");
            safeSend(sock, pack, length, 0);
        }
    }
}

// ========================================================================= //

int sendEnd(int sock){
    char pack[PSIZE_END];
    pack[0]=PTYPE_END;
    debug_print("%s\n", "Sending END packet.. ");
    if(safeSend(sock, &pack, PSIZE_END, 0) < 0){
        perror(ERR_SEND);
        return 1;
    };
    return 0;
}

// ========================================================================= //

int readQuit(int sock, int32_t playerId){
    char pack[PSIZE_QUIT-1];

    // Pārbauda vai spēlētājs nemēģina atvienot kādu citu spēlētāju
    if(playerId == batoi(&pack[0])){
        debug_print("Client with Id: %d disconnecting..\n", batoi(&pack[1]));
        return 0;
    }
    debug_print("Client sent wrong Id: %d in QUIT pack\n", batoi(&pack[1]));
    return 1;
}

// ========================================================================= //

void readMessage(int sock, int32_t playerId){
    char header[8];
    int32_t messageSize;
    int32_t messageId; // Spēlētāja id ziņas paketē
    char* message;

    serverRecv(sock, &header, 8, 0);

    messageId = batoi(header);
    messageSize = batoi(&header[4]);

    printf("Player id: %d Msg size: %d\n", messageId, messageSize);
    message = allocPack(messageSize+1);
    serverRecv(sock, message, messageSize, 0);
    message[messageSize] = '\0';

    printf("MESSAGE2: %s, from client: %d\n",message, messageId);

    // Pārsūta ziņu čatā
    int packSize = 1+4+4+messageSize;
    char* pack = allocPack(packSize);
    pack[0] = PTYPE_MESSAGE;                //0. baits
    itoba(playerId, &pack[1]);              //1. - 4. baits
    itoba(messageSize, &pack[5]);            //5. - 8. baits
    strncpy(&pack[9], message, messageSize); //9. - x. baits
    sendBroadcast(pack, packSize, true);
    free (message);
    free (pack);
}

// ========================================================================= //

// Atgriež  0 - viss kārtībā
//          1 - kļūda
//          2 - spēlētājs atvienojās
int readTCPPacket(int sock, int32_t playerId){
    debug_print("Receiving TCP packets.. playerID: %d\n", playerId);
    char packtype;
    if(recv(sock, &packtype, 1, MSG_DONTWAIT) < 0){
        return 1;
    }
    if(packtype == PTYPE_MESSAGE){
        readMessage(sock, playerId);
        return 0;
    }
    if(packtype == PTYPE_QUIT){
        readQuit(sock, playerId);
        return 2;
    }
    debug_print("Received <UNKNOWN> TCP packtype: %d playerID: %d\n", (int)packtype, playerId);
    return 1;
}

// ========================================================================= //

//TODO: Randomizēt
void setSpawnPoint(float *x, float *y){
    // Iegūst samērā nejaušu punktu 2D masīvā
    srand(randSeed++); // Maina rand seed ik reiz pirms izmantošanas
    int rrow = rand() % MAPHEIGHT;
    int rcol = rand() % MAPWIDTH;
    printf("Random Spawn spot in MAP array: [%2d;%2d]\n", rrow, rcol);

    // Pārbauda vai vieta no šī punkta uz priekšu ir bīva
    for (int i = rrow; i < MAPHEIGHT; ++i){
        for (int j = rcol; j < MAPWIDTH; ++j){
            if(MAP[i][j] == MTYPE_EMPTY){
                *x = j; *y = i;
                return;
            }
        }
    }

    // Pārbauda vai vieta no šī punkta uz atpakaļu ir bīva
    for (int i = rrow; i >= 0; --i){
        for (int j = rcol; j >= 0 ; --j){
            if(MAP[i][j] == MTYPE_EMPTY){
                *x = j; *y = i;
                return;
            }
        }
    }
    //TODO: Do something.
    debug_print("%s\n", "Could not find any free spawn place.");
}

// ========================================================================= //

int getPlayerType(){
    // Izlīdzina pacman un ghost skaitu uz visiem spēlētājiem 
    // sākotnēji pieslēdzoties
    // NOTE: Ar laiku varētu izdomāt ko interesantāku
    return playerCount % 2 ? PLTYPE_GHOST : PLTYPE_PACMAN;
}

// ========================================================================= //

void initNewPlayer(int id, int type, char *name){
    // Inicializē objektu
    object_t newPlayer;
    newPlayer.type = type;
    newPlayer.id = id;
    strcpy(newPlayer.name, name);
    newPlayer.points = 0;
    setSpawnPoint(&newPlayer.x, &newPlayer.y);
    newPlayer.state = PLSTATE_LIVE;
    newPlayer.mdir = DIR_NONE;
    newPlayer.disconnected = 0;
    newPlayer.sockets.udp = 0;
    newPlayer.sockets.tcp = 0;

    // Pievieno sarakstam
    addObjectNodeEnd(&STATE, createObjectNode(&newPlayer));
};

// ========================================================================= //

void addObjectNode(objectNode_t **start, objectNode_t *newNode){
    playerCount++;
    // If List is empty
    if(*start == 0){
        *start = newNode;
        return;
    }
    // If List not empty
    newNode->next = *start;
    *start = newNode;
}

// ========================================================================= //

void addObjectNodeEnd(objectNode_t **start, objectNode_t *newNode){
    playerCount++;
    // If List is empty
    if(*start == 0){
        *start = newNode;
        return;
    }
    // If List not empty
    for(objectNode_t *current = *start; current != 0; current = current->next){
        if(current->next == 0){
            current->next = newNode;
            return;
        }
    }
}

// ------------------------------------------------------------------------- //

objectNode_t *createObjectNode(object_t* gameObj){
    objectNode_t *newNode;
    newNode = (objectNode_t*)malloc(sizeof(objectNode_t));
    // If did not allocate memory
    if( newNode == NULL ){
        printf(ERR_MALLOC);
        exit(1);
    }
    newNode->next = 0;
    memcpy(&(newNode->object), gameObj, sizeof(object_t));
    return newNode;
};

// ------------------------------------------------------------------------- //

void deleteObjectWithId(objectNode_t **start, int id){
    for(objectNode_t *current = *start; current != 0; current = current->next){
        if(current->object.id == id){
            if(deleteObjectNode(start, current)){
                debug_print("Could not delete node with id: %d\n", id);
            };
            playerCount--;
            return;
        }
    }
}

// ------------------------------------------------------------------------- //

int deleteObjectNode(objectNode_t **start, objectNode_t *node){
    objectNode_t* tmp = *start;
    // Node is head
    if(*start == node){
        *start = (*start)->next;
        free(tmp);
        return 0;
    }
    // Node is not head
    for(objectNode_t *current = (*start)->next; current != 0; current = current->next){
        if(current == node){
            tmp->next = current->next;
            free(current);
            return 0;
        }
        tmp = tmp->next;
    }
    return 1;
}

// ------------------------------------------------------------------------- //

void printPlayerList(objectNode_t *start){
    printf("%s\n", "===========PLAYERS===========");
    for(objectNode_t *current = start; current != 0; current = current->next){
        printf("ID: %1d Type: %d X: %2f Y: %2f ST: %d\nName: %s Points: %d MDir: %d\n\n",\
           current->object.id, current->object.type, current->object.x, \
           current->object.y, current->object.state, current->object.name, \
           current->object.points, current->object.mdir);
    }
    printf("Total count: %d\n", playerCount);
    printf("%s\n", "=============================");
}

// ------------------------------------------------------------------------- //

object_t* getPlayer(objectNode_t *start, int id){
    for(objectNode_t *current = start; current != 0; current = current->next){
        if(current->object.id == id){
            return &(current->object);
        }
    }
    return 0;
}

// ========================================================================= //

void resetGame(){
    // Ielasām jaunu mani no karšu kolekcijas
    // TODO: izveidot karšu kolekciju
    getNewMap(mapFileName);
    // Inicializē spēlētāju punktus uz nulli, 
    // maina spēlētāju spawn atrašanās vietas
    resetPlayers(STATE);
}

// ========================================================================= //

void getNewMap(char* filename){
    FILE *mapFile;
    int type,x,y;   // priekš kartes ielasīšanas; objekta tips, x ,y

    if((mapFile = fopen(filename, "r")) == NULL){
        Die("Could not open map file");    
    }    

    debug_print("%s\n", "Reading Map File...");
    if(fscanf(mapFile, "%d %d ", &MAPWIDTH, &MAPHEIGHT) == 0){
        Die("Could not read map's widht and height");
    }
    debug_print("Map width: %d, Map height: %d\n", MAPWIDTH, MAPHEIGHT);

    // Atbrīvojam iepriekšējās kartes atmiņu
    if(MAP != 0){
        free(MAP);
    }
    MAP = allocateGameMap(MAPWIDTH, MAPHEIGHT);

    while(fscanf(mapFile, " %d %d %d ", &type, &x, &y) > 0) {
        MAP[x][y] = type;
    }
    if(fclose(mapFile) != 0){
        printf("%s\n", "Could not close mapFile");
    }
    printMap(MAP,MAPWIDTH,MAPHEIGHT);
}


// ========================================================================= //


void resetPlayers(objectNode_t *start){
    for(objectNode_t *current = start; current != 0; current = current->next){
            current->object.state = PLSTATE_LIVE;
            setSpawnPoint(&current->object.x,&current->object.y);
            current->object.points = 0;
            current->object.mdir = DIR_NONE;
    }
    debug_print("%s\n", "Players are reset");
}

// ========================================================================= //

int isGameEnd(){
    return (!isPointsLeft() || !isAnyPacmanLive(STATE)) ? 1 : 0;
}

// ------------------------------------------------------------------------- //

int isPointsLeft(){
    for (int i = 0; i < MAPHEIGHT; ++i){
        for (int j = 0; j < MAPWIDTH; ++j){
            if(MAP[i][j] == MTYPE_DOT){
                return 1;
            }
        }
    }
    debug_print("%s\n", "No Points left on the Map!");
    return 0;
}

// ------------------------------------------------------------------------- //

int isAnyPacmanLive(objectNode_t *start){
    for(objectNode_t *current = start; current != 0; current = current->next){
        if(current->object.state == PLSTATE_LIVE &&\
            current->object.type == PLTYPE_PACMAN)
        {

            return 1;           
        }
    }
    debug_print("%s\n", "No Packman left alive!");
    return 0;
}

// ========================================================================= //

void setPlayerDisconnected(int32_t playerId){
    for(objectNode_t *current = STATE; current != 0; current = current->next){
        if(current->object.id == playerId){
            current->object.disconnected = 1;
            return;
        }
    }
}
