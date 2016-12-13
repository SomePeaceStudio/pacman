#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

// ========================================================================= //

#define MAXPENDING 5    /* Max connection requests */
#define BUFFSIZE 32

// #define MAPWIDTH 3
// #define MAPHEIGHT 3

#define DEBUG 1
#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

// ------------------------------------------------------------------------- //

typedef struct {
    char pactype;
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

typedef struct objectNode{
    object_t object;
    struct objectNode *next;
} objectNode_t;

typedef struct {
    int id;
    char* nick;
    int score;
} score_t;

// typedef struct {
//     char pactype;
//     // int mapsize[2];
//     int width;
//     int height;
//     int map[MAPWIDTH][MAPHEIGHT];
// } mappac_t;

// ------------------------------------------------------------------------- //

int ID = 0;
// int MAP[MAPWIDTH][MAPHEIGHT] = {{0,0,0},{0,1,0},{0,0,0}};
// object_t STATE[MAPWIDTH][3] = {{{},{},{}},{{},{},{}},{{},{},{}}};
objectNode_t *STATE;
int stateObjCount = 0;
int MAPHEIGHT;
int MAPWIDTH;

// ========================================================================= //

void Die(char *mess) { perror(mess); exit(1); }
int getId() { return ID++; }
void printMappac(char* mappac);
int deleteObjectNode(objectNode_t **start, objectNode_t *node);
void updateState(int id,int x,int y);
void sendStateUpdate(int sock);

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
           current->object.y, current->object.status);
    }
    printf("Total count: %d\n", stateObjCount);
}

// ========================================================================= //

void HandleClient(int sock) {
    char buffer[BUFFSIZE];
    int received = -1;
    int playerId;

    char pactype;
    /* Receive message type*/
    printf("%s\n", "Receiving packtype ...");
    if ((received = recv(sock, &pactype, sizeof(pactype), 0)) < 0) {
        Die("Failed to receive initial bytes from client");
    }

    /* Wait for join request */
    while (received > 0) {
        printf("Just received type: %c\n", pactype);
        // Client wants to join the game
        if( pactype == 'J'){
            /* Send back acknowledgement and client ID */
            playerId = getId();
            join_t acknowlgmt;
            acknowlgmt.pactype = 'J';
            acknowlgmt.response = 'K';
            acknowlgmt.id = playerId;
            if (send(sock, &acknowlgmt, sizeof(acknowlgmt), 0) != sizeof(acknowlgmt)) {
                Die("Failed to send bytes to client");
            }
            break;
        }
        printf("Just received unknown type: %c\n", pactype);
    }

    // Send Map
    // mappac_t mappac = {'M', MAPWIDTH, MAPHEIGHT, {{1,2,3},{4,5,6},{7,8,9}}};
    debug_print("%s\n", "Sending map...");
    // int mappack2size = (int)(sizeof(char)+sizeof(int)*2+sizeof(MAP));
    int mappack2size = (int)(sizeof(char)+sizeof(int)*2);
    char* mappac2 = (char*)calloc(1,mappack2size);
    mappac2[0] = 'M';
    mappac2[1] = MAPWIDTH;
    mappac2[5] = MAPHEIGHT;
    //memcpy(&mappac2[9],&MAP, sizeof(MAP));


    debug_print("Widht: %d\n", mappac2[1]);
    debug_print("height: %d\n", mappac2[5]);
    //printMappac(&mappac2[9]);
    debug_print("Packsize: %d\n", mappack2size);

    if (send(sock, mappac2, mappack2size, 0) != mappack2size) {
        Die("Failed to send bytes to client");
    }

    // Add player to the game
    debug_print("Adding player to STATE with id: %d ...\n", playerId);
    object_t player = { '2', playerId, 1, 1};
    addObjectNodeEnd(&STATE, createObjectNode(&player));
    debug_print("%s\n", "New State:");
    printObjectNodeList(STATE);

    // Send State Update 
    sendStateUpdate(sock);

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
        // /* Send back received data */
        // if (send(sock, buffer, received, 0) != received) {
        //     Die("Failed to send bytes to client");
        // }
        // /* Check for more data */
        // if ((received = recv(sock, buffer, BUFFSIZE, 0)) < 0) {
        //     Die("Failed to receive additional bytes from client");
        // }
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

    // int mapWidth, mapHeight;
    object_t obj;
    debug_print("%s\n", "Reading Map File...");
    if(fscanf(mapFile, "%d %d ", &MAPWIDTH, &MAPHEIGHT) == 0){
        Die("Could not read map's widht and height");
    }
    debug_print("Map width: %d, Map height: %d\n", MAPWIDTH, MAPHEIGHT);

    while(fscanf(mapFile, " %c %lf %lf ", &(obj.type), &(obj.x), (&obj.y)) > 0) {
        obj.id = getId();
        obj.status = 0;
        addObjectNode(&STATE, createObjectNode(&obj));
    }
    if(fclose(mapFile) != 0){
        printf("%s\n", "Could not close mapFile");
    }
    printObjectNodeList(STATE); 
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

void printMappac(char* mappac){
    for (int i = 0; i < MAPHEIGHT*MAPWIDTH; ++i){
        printf("%3d", *((int*)mappac+i) );
        if((i+1) % MAPWIDTH == 0){
            printf("\n");
        }        
    }
}

// ========================================================================= //

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

void sendStateUpdate(int sock){
    int newStateSize = (int)(sizeof(char)+sizeof(int)+sizeof(object_t)*stateObjCount);
    char* newState = (char*)calloc(1,newStateSize);
    newState[0] = 'U';
    newState[1] = stateObjCount;
    int i = 5;
    for(objectNode_t *current = STATE; current != 0; current = current->next){
        memcpy(&newState[i],&(current->object), sizeof(object_t));
        i += sizeof(object_t);
    }
    debug_print("%s\n", "Sending State Update...");
    debug_print("Size: %d PACKET: %c%d ...\n",newStateSize, newState[0], newState[1]);
    if (send(sock, newState, newStateSize, 0) != newStateSize) {
            debug_print("%s\n", "Could not send all packets!!");
            //Die("Failed to send bytes to client");
    }
    free(newState);
    newState = 0;
    printf("%s\n", "Sent State to the client!");
}












// mappac_t mappac;
    // mappac.pactype = 'M';
    // mappac.width = MAPWIDTH;
    // mappac.height = MAPHEIGHT;
    // mappac.map = {{0,0,0},{0,0,0},{0,0,0}};