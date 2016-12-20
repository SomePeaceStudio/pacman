#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <math.h>


// Shared functions for client and server
#include "../shared/shared.h"

#define MAXPENDING 5    /* Max connection requests */

// ------------------------------------------------------------------------- //

typedef struct objectNode{
    object_t object;
    struct objectNode *next;
} objectNode_t;

// ------------------------------------------------------------------------- //

int ID = 1;
objectNode_t *STATE;

char** MAP;
int stateObjCount = 0;
int MAPHEIGHT;
int MAPWIDTH;
pthread_t  tid;   // Second thread


// ========================================================================= //

int getId() { return ID++; }
void addObjectNode(objectNode_t **start, objectNode_t *newNode);
void addObjectNodeEnd(objectNode_t **start, objectNode_t *newNode);
objectNode_t *createObjectNode(object_t* gameObj);
void deleteObjectWithId(objectNode_t **start, int id);
int deleteObjectNode(objectNode_t **start, objectNode_t *node);
void printObjectNodeList(objectNode_t *start);
void updateState();
void sendStateUpdate(int sock);
void sendMapUpdate(int sock);
void initNewPlayer(int id, int type, char *name);
void setSpawnPoint(float *x, float *y);
void sendPlayersState(int sock);
void* actionTherad();   // Funkcija priekš klienta gājienu apstrādes


// ========================================================================= //

void addObjectNode(objectNode_t **start, objectNode_t *newNode){
    stateObjCount++;
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
    stateObjCount++;
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
        printf("%s\n", "Error: Could not allocate memory");
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
            stateObjCount--;
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

void printObjectNodeList(objectNode_t *start){
    printf("%s\n", "===========PLAYERS===========");
    for(objectNode_t *current = start; current != 0; current = current->next){
        printf("ID: %1d Type: %d X: %2f Y: %2f ST: %d\nName: %s Points: %d MDir: %d\n\n",\
           current->object.id, current->object.type, current->object.x, \
           current->object.y, current->object.state, current->object.name, \
           current->object.points, current->object.mdir);
    }
    printf("Total count: %d\n", stateObjCount);
    printf("%s\n", "=============================");
}

// ------------------------------------------------------------------------- //

object_t* getObject(objectNode_t *start, int id){
    for(objectNode_t *current = start; current != 0; current = current->next){
        if(current->object.id == id){
            return &(current->object);
        }
    }
    return 0;
}

// ========================================================================= //

void* actionTherad(void *parm){
    int sock = (int)(intptr_t)parm;
    char* pack = allocPack(PSIZE_MOVE-1);
    char packtype;
    object_t *player;
    
    while(1){
        packtype = receivePacktype(sock);

        if((int)packtype == PTYPE_MOVE){
            safeRecv(sock, pack, PSIZE_MOVE-1, 0);
            debug_print("%s\n", "Done receiving!");
            debug_print("Id: %d\n", *(int*)(pack));
            debug_print("mdir: %d\n", *(pack+4));

            player = getObject(STATE, *(int*)(pack));
            if(player!=0){
                player->mdir = *(pack+4);
                continue;
            }
            debug_print("Could not find player to move, id: %d\n" , *(int*)(pack));
        }else{
            //TODO: Do something.
            break;
        }
    }
    debug_print("%s\n", "Exiting actionTherad...");
    if(pack!=0){
        free(pack);
    }
};
void HandleClient(int sock) {
    char *pack;
    char packtype;
    int packSize;
    int playerId;
    char playerName[MAX_NICK_SIZE+1];


    //------- JOIN / ACK -------//
    packtype = receivePacktype(sock);
    if((int)packtype == PTYPE_JOIN){
        // Get User Nickname
        safeRecv(sock, &playerName, MAX_NICK_SIZE,0);
        playerName[MAX_NICK_SIZE] = '\0';
        debug_print("Received playerName: %s\n", playerName);


        //TODO: Dinamiski izvēlēties PLTYPE
        playerId = getId();
        initNewPlayer(playerId, PLTYPE_PACMAN, playerName);

        // Send ACK
        debug_print("%s\n", "Sending ACK for JOIN");
        pack = allocPack(PSIZE_ACK);
        pack[0] = PTYPE_ACK;
        memcpy(&pack[1], &playerId, sizeof(playerId));
        safeSend(sock, pack, PSIZE_ACK, 0);
        free(pack);

    }else{
        //TODO: do something;
    }


    //------- START -------//
    debug_print("%s\n", "Sending START packet...");
    pack = allocPack(PSIZE_START);
    pack[0] = 2;
    pack[1] = MAPHEIGHT;
    pack[2] = MAPWIDTH;
    pack[3] = getObject(STATE, playerId)->x;
    pack[4] = getObject(STATE, playerId)->y;
    safeSend(sock, pack, PSIZE_START, 0);
    free(pack);
    
    // Apstrādājam klienta gājienus atsevišķā threadā
    pthread_create(&tid, NULL, actionTherad, (void*)(intptr_t) sock);

    // Main GAME loop
    while(1){
        // Send Map Update 
        sendMapUpdate(sock);

        // Send Players
        sendPlayersState(sock);

        // Pavirza spēlētājus uz priekšu
        updateState();

        sleep(1);
    }

    close(sock);
    debug_print("%s\n", "Closed socket!");
    debug_print("%s\n", "Updating state..");
    deleteObjectWithId(&STATE, playerId);
    printObjectNodeList(STATE);

}


int main(int argc, char *argv[]) {
    printf("%f\n", floorf(-0.5));
    int serversock, clientsock;
    struct sockaddr_in gameserver, gameclient;

    if (argc != 3) {
      fprintf(stderr, "USAGE: gameserver <port> <mapfile>\n");
      exit(1);
    }
    /* Create the TCP socket */
    if ((serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      Die("Failed to create socket");
    }
    /* Construct the server sockaddr_in structure */
    memset(&gameserver, 0, sizeof(gameserver));       /* Clear struct */
    gameserver.sin_family = AF_INET;                  /* Internet/IP */
    gameserver.sin_addr.s_addr = htonl(INADDR_ANY);   /* Incoming addr */
    gameserver.sin_port = htons(atoi(argv[1]));       /* server port */

     /* Bind the server socket */
    if (bind(serversock, (struct sockaddr *) &gameserver,
                               sizeof(gameserver)) < 0) {
        Die("Failed to bind the server socket");
    }

    // -------------INITIALIZE MAP------------------------------------------ //
    FILE *mapFile;
    if((mapFile = fopen(argv[2], "r")) == NULL){
        Die("Could not open mapFile");
        return 1;
    }    

    object_t obj;
    debug_print("%s\n", "Reading Map File...");
    if(fscanf(mapFile, "%d %d ", &MAPWIDTH, &MAPHEIGHT) == 0){
        Die("Could not read map's widht and height");
    }
    debug_print("Map width: %d, Map height: %d\n", MAPWIDTH, MAPHEIGHT);

    MAP = allocateGameMap(MAPWIDTH, MAPHEIGHT);

    int type,x,y;
    while(fscanf(mapFile, " %d %d %d ", &type, &x, &y) > 0) {
        MAP[x][y] = type;
    }
    if(fclose(mapFile) != 0){
        printf("%s\n", "Could not close mapFile");
    }

    printMap(MAP,MAPWIDTH,MAPHEIGHT);

    // --------------------------------------------------------------------- //

    /* Listen on the server socket */
    if (listen(serversock, MAXPENDING) < 0) {
        Die("Failed to listen on server socket");
    }

    /* Run until cancelled */
    while (1) {
        unsigned int clientlen = sizeof(gameclient);
        /* Wait for client connection */
        if ((clientsock =
           accept(serversock, (struct sockaddr *) &gameclient,
                  &clientlen)) < 0) {
            Die("Failed to accept client connection");
        }
        fprintf(stdout, "Client connected: %s\n", 
                    inet_ntoa(gameclient.sin_addr));
        HandleClient(clientsock);
    }
}

// ========================================================================= //

void updateState(){
    float *x, *y;
    int mdir;
    int canMove;        // Vai gājienu var izdarīt (vai priekšā nav siena);
    float speed = 1;  // Vina gājiena lielums vienā game-tick
    float xSpeed = 0, ySpeed = 0; // Spēlētāja x un y ātrums;
    char *nextMapTile;   // Kartes lauciņš uz kuru spēlētājs grasās pārvietoties
    float tileSize = 1; // vienas rūtiņas platums


    // Veicam visas kustības visiem spēlētājiem atkarībā no to kursības virziena
    // Apskatām visas kolīzijas ar kartes objektiem
    for(objectNode_t *current = STATE; current != 0; current = current->next){
        x = &(current->object.x);
        y = &(current->object.y);
        mdir = (int)(current->object.mdir);

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
            *x+=xSpeed-MAPHEIGHT;
            continue;  
        }
        if((int)floorf(*x+xSpeed+tileSize/2) < 0){
            *x+=xSpeed+MAPHEIGHT;
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
        //TODO: Pārbaudīt kolīzijas Pacman/Ghost
        if(canMove){
            *x+=xSpeed;
            *y+=ySpeed; 
        }
    }
}

// ========================================================================= //

void sendPlayersState(int sock){
    int packSize;
    char* pack;

    packSize = 1 + 4 + stateObjCount*OSIZE_PLAYER;
    pack = allocPack(packSize);
    pack[0] = PTYPE_PLAYERS;
    // pack[1] = stateObjCount;
    *(int*)(pack+1) = stateObjCount;

    char* currObj = &pack[5]; // Norāda uz sākumvietu, kur rakstīt objektu
    for(objectNode_t *current = STATE; current != 0; current = current->next){
        *(int*)currObj  = current->object.id;
        *(float*)(currObj+4) = current->object.x;
        *(float*)(currObj+8) = current->object.y;
        *(currObj+12) = current->object.state;
        *(currObj+13) = current->object.type;
        currObj += 14;
    }

    // Send MAP packet
    debug_print("Sending %d PLAYERS... packSize: %d\n", *(int*)(pack+1), packSize);
    printObjectNodeList(STATE);
    safeSend(sock, pack, packSize, 0);
    if(pack != 0){
        free(pack);
    }
}

// ========================================================================= //

void sendMapUpdate(int sock){
    int packSize;
    char* pack;

    packSize = 1 + MAPWIDTH * MAPHEIGHT;
    pack = allocPack(packSize);
    pack[0] = PTYPE_MAP;
    memcpy(&pack[1], *MAP, MAPWIDTH * MAPHEIGHT);

    printMappacPretty(&pack[1], MAPWIDTH, MAPHEIGHT);
    // Send MAP packet
    debug_print("%s\n", "Sending MAP packet...");
    safeSend(sock, pack, packSize, 0);
    if(pack!=0){
        free(pack);
    }
}

// ========================================================================= //

void setSpawnPoint(float *x, float *y){
    for (int i = 0; i < MAPHEIGHT; ++i){
        for (int j = 0; j < MAPWIDTH; ++j){
            if(MAP[i][j] == MTYPE_EMPTY){
                *x = j; *y = i;
                return;
            }
        }
    }
    debug_print("%s\n", "Could not find any free spawn place.");
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
    int status = PLSTATE_LIVE;
    newPlayer.mdir = DIR_NONE;
    // Pievieno sarakstam
    addObjectNodeEnd(&STATE, createObjectNode(&newPlayer));
};

// ========================================================================= //

