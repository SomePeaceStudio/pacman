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

// ========================================================================= //

void Die(char *mess) { perror(mess); exit(1); }

// ========================================================================= //

void safeSend(int sockfd, const void *buf, size_t len, int flags){
    if (send(sockfd, buf, len, flags) != len) {
        Die("Mismatch in number of sent bytes");
    }
}

// ========================================================================= //

void safeRecv(int sockfd, void *buf, size_t len, int flags){
    int received;
    if ((received = recv(sockfd, buf, len, flags)) != len) {
        debug_print("Received: %2d bytes, should be: %d\n", received, (int)len);
        Die("Failed to receive bytes from server");
    }
    debug_print("Received: %2d bytes\n", received);
    
}

// ========================================================================= //

char* allocPack(int size){
    // Init pack with 0 byte
    char* pack = (char*)calloc(1, size);
    // If did not allocate memory
    if( pack == NULL ){
        Die("Error: Could not allocate memory");
    }
    return pack;
}

// ========================================================================= //

char receivePacktype(int sock){
    char packtype;
    safeRecv(sock, &packtype, 1, 0);
    debug_print("Received packtype: %d\n", packtype);
    return packtype;
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

// Encoding
// 0 None
// 1 Dot
// 2 Wall
// 3 PowerPellet
// 4 Invincibility
// 5 Score

char* translateType(int type){
    if (type == '0'){
        return "~";
    }
    if (type == '1'){
        return ANSI_COLOR_YELLOW "." ANSI_COLOR_RESET;
    }
    if (type == '2'){
        return "=";
    }
    if (type == '3'){
        return ANSI_COLOR_CYAN "*" ANSI_COLOR_RESET;
    }
    if (type == '4'){
        return ANSI_COLOR_MAGENTA "#" ANSI_COLOR_RESET;
    }
    if (type == '5'){
        return ANSI_COLOR_GREEN "$" ANSI_COLOR_RESET;
    }
    return "E"; // E for Error
}

// ========================================================================= //

// For debugging on server/client
void printMap(int** map, int width, int height){
    for (int i = 0; i < height; ++i){
        for (int j = 0; j < width; ++j){
           printf(" %s", translateType(map[i][j]));
        }
        printf("\n");
    }
};

// ========================================================================= //
