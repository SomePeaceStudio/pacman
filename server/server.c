#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

// Shared functions for client and server
#include "../shared/shared.h"

#define MAXPENDING 5    /* Max connection requests */

// ------------------------------------------------------------------------- //

typedef struct objectNode{
    object_t object;
    struct objectNode *next;
} objectNode_t;

// ------------------------------------------------------------------------- //

int ID = 0;
objectNode_t *STATE;

char** MAP;
int stateObjCount = 0;
int MAPHEIGHT;
int MAPWIDTH;

// ========================================================================= //

int getId() { return ID++; }
void addObjectNode(objectNode_t **start, objectNode_t *newNode);
void addObjectNodeEnd(objectNode_t **start, objectNode_t *newNode);
objectNode_t *createObjectNode(object_t* gameObj);
void deleteObjectWithId(objectNode_t **start, int id);
int deleteObjectNode(objectNode_t **start, objectNode_t *node);
void printObjectNodeList(objectNode_t *start);
void updateState(int id,int x,int y);
void sendStateUpdate(int sock);
void sendMapUpdate(int sock);
void initNewPlayer(int id, int type, char *name);
void setSpawnPoint(double *x, double *y);
void sendPlayersState(int sock);


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
    for(objectNode_t *current = start; current != 0; current = current->next){
        printf("ID: %1d Type: %c X: %2f Y: %2f ST: %d\n",\
           current->object.id, current->object.type, current->object.x, \
           current->object.y, current->object.state);
    }
    printf("Total count: %d\n", stateObjCount);
}

// ------------------------------------------------------------------------- //

object_t* getObject(objectNode_t *start, int id){
    for(objectNode_t *current = start; current != 0; current = current->next){
        if(current->object.id == id){
            return &(current->object);
        }
    }
}

// ========================================================================= //

void HandleClient(int sock) {
    char *pack;
    char pactype;
    int packSize;
    int playerId;
    char playerName[MAX_NICK_SIZE+1];


    //------- JOIN / ACK -------//
    pactype = receivePacktype(sock);
    if((int)pactype == PTYPE_JOIN){
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


    // Send Map Update 
    sendMapUpdate(sock);

    // Send Players
    sendPlayersState(sock);


    return;


    // Main GAME loop
    while(1){
        // Get actions from clients
        char pactype = '0';
        /* Receive message type*/
        debug_print("%s\n", "Receiving packtype...");
        if (recv(sock, &pactype, sizeof(pactype), 0) < 0) {
            Die("Failed to receive packtype from server");
        }
        debug_print("Received pactype: %c\n", pactype);

        if(pactype == 'A'){
            debug_print("%s\n", "Receiving client action...");
            int move[3];
            if (recv(sock, &move, sizeof(move), 0) < 1) {
                Die("Failed to receive updateSize from server");
            }
            debug_print("Client ID%2d trying to move (%d,%d)\n", move[0],move[1],move[2]);
            updateState(move[0],move[1],move[2]);
        }

        // Send State Update
        sendStateUpdate(sock);
        printObjectNodeList(STATE); 



        // Send Only State Updates
        // State Update pack
        if(pactype == '0'){
            break;
        } 
    }

    close(sock);
    debug_print("%s\n", "Closed socket!");
    debug_print("%s\n", "Updating state..");
    deleteObjectWithId(&STATE, playerId);
    printObjectNodeList(STATE);

}


int main(int argc, char *argv[]) {
    int serversock, clientsock;
    struct sockaddr_in echoserver, echoclient;

    if (argc != 3) {
      fprintf(stderr, "USAGE: echoserver <port> <mapfile>\n");
      exit(1);
    }
    /* Create the TCP socket */
    if ((serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      Die("Failed to create socket");
    }
    /* Construct the server sockaddr_in structure */
    memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
    echoserver.sin_family = AF_INET;                  /* Internet/IP */
    echoserver.sin_addr.s_addr = htonl(INADDR_ANY);   /* Incoming addr */
    echoserver.sin_port = htons(atoi(argv[1]));       /* server port */

     /* Bind the server socket */
    if (bind(serversock, (struct sockaddr *) &echoserver,
                               sizeof(echoserver)) < 0) {
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
        unsigned int clientlen = sizeof(echoclient);
        /* Wait for client connection */
        if ((clientsock =
           accept(serversock, (struct sockaddr *) &echoclient,
                  &clientlen)) < 0) {
            Die("Failed to accept client connection");
        }
        fprintf(stdout, "Client connected: %s\n", 
                    inet_ntoa(echoclient.sin_addr));
        HandleClient(clientsock);
    }
}

// ========================================================================= //

//UNUSED 
void updateState(int id,int x,int y){
    objectNode_t* client = 0;
    for(objectNode_t *current = STATE; current != 0; current = current->next){
        if(current->object.id == id){
            client = current;
            debug_print("    %s\n", "Clinet has been found!");
            break;
        }
    }
    if (client == 0){
        debug_print("    %s\n", "Could not find client!");
        return;
    }
    short canMove = 1;
    for(objectNode_t *current = STATE; current != 0; current = current->next){
        //TODO: calculate in decimals
        if(current->object.x == client->object.x+x\
            && current->object.y == client->object.y+y){
            debug_print("    %s\n", "Found collision object!");
            // Collides
            // If client is a packman
            if(client->object.type == '2'){
                // If collides with ghost
                if(current->object.type == '4'){
                    debug_print("%s\n", "Packman collides with Ghost.");
                    deleteObjectWithId(&STATE, client->object.id);
                    canMove = 0;
                    return;
                }  
                // If collides with point
                if(current->object.type == '3'){
                    debug_print("%s\n", "Packman collides with Point.");
                    // TODO: Update Score table, add point to Packman
                    // Set point to empty space
                    current->object.type = '0';
                }
                // If collides with powerup
                if(current->object.type == '5'){
                    debug_print("%s\n", "Packman collides with Powerup.");
                    // TODO: Do something
                }   
                // If collides with wall
                if(current->object.type == '1'){
                    debug_print("%s\n", "Packman collides with wall.");
                    canMove = 0;
                }                  
            }
            // If client is a Ghost
            if(client->object.type == '4'){
                // If collides with Packman
                if(current->object.type == '2'){
                    debug_print("%s\n", "Ghost collides with Packman.");
                    // Delete packman
                    deleteObjectWithId(&STATE, current->object.id);
                    // TODO: Add one score to Ghost
                    break;
                }
                // If collides with wall
                if(current->object.type == '1'){
                    debug_print("%s\n", "Ghost collides with wall.");
                    canMove = 0;
                }  
            }
            
        }
    }
    if(canMove){
        debug_print("Moving client with id %2d\n", client->object.id);
        client->object.x += x;
        client->object.y += y;
    }

}

// ========================================================================= //

void sendPlayersState(int sock){
    int packSize;
    char* pack;

    packSize = 1 + stateObjCount*OSIZE_PLAYER;
    pack = allocPack(packSize);
    pack[0] = PTYPE_PLAYERS;
    *(int*)(pack+1) = stateObjCount;

    char* currObj = &pack[5]; // Norāda uz sākumvietu, kur rakstīt objektu
    for(objectNode_t *current = STATE; current != 0; current = current->next){
        *(int*)currObj  = current->object.id;
        *(float*)(currObj+4) = current->object.x;
        *(float*)(currObj+8) = current->object.y;
        *(currObj+12) = current->object.state;
        *(currObj+13) = current->object.type;
        currObj += 15;
    }

    // Send MAP packet
    debug_print("%s\n", "Sending PLAYERS packet...");
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

void setSpawnPoint(double *x, double *y){
    for (int i = 0; i < MAPHEIGHT; ++i){
        for (int j = 0; j < MAPWIDTH; ++j){
            if(MAP[i][j] == MTYPE_EMPTY){
                *x = i; *y = j;
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
    // Pievieno sarakstam
    addObjectNodeEnd(&STATE, createObjectNode(&newPlayer));
};

// ========================================================================= //

