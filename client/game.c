#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <pthread.h>

#include "game.h"
#include "tile.h"
#include "player.h"
#include "network.h"
#include "../shared/hashmap.h"

//72, jo serveris vairāk nesūtīs (1024 baitu buferī vairāk nesaiet)
#define PLAYER_COUNT_MAX 72

//Nodefinē hashmap priekš int atslēgām un Player tipa datiem
HASHMAP_FUNCS_DECLARE(int, int32_t, Player);
HASHMAP_FUNCS_CREATE(int, int32_t, Player);


//Struktūra, ko padot pthread_create kā argumentu
typedef struct {
    sockets_t sockets;
    uint32_t tileCount;
    uint32_t levelWidth;
    Player* me;
    Tile* tiles;
    struct hashmap* hm_players;
} thread_args_t;

void waitForStart();
void game_handleInput(int sock, int playerId, SDL_KeyboardEvent* event);
void game_showMainWindow(Player* me, sockets_t* sock, struct sockaddr_in* serverAddress);

//Funkcijas, ko izpildīs divi papildus pavedieni
void* thread_handleUdpPackets(void* arg);
void* thread_handleTcpPackets(void* arg);


//TODO: pievienot time-out gadījumam, ja serveris neatbild
//TODO: pārcelt uz citu pavedienu, lai varētu aizvērt logu, ja serveris neatbild
void waitForStart(
    sockets_t* sock, Player* player, int* mapWidth, int* mapHeight){
        
    char packtype;
    unsigned char *pack;
    
    packtype = receivePacktype(sock->tcp);
    if(packtype == PTYPE_START){
        pack = allocPack(PSIZE_START-1);
        safeRecv(sock->tcp, pack, PSIZE_START-1, 0);
        *mapHeight = (int)pack[0];
        *mapWidth = (int)pack[1];
        player->x = (int32_t)pack[2];
        player->y = (int32_t)pack[3];
    }
    
    free(pack);
}

void game_handleInput(int sock, int playerId, SDL_KeyboardEvent* event) {
    char direction = DIR_UNDEFINED;
    switch (event->keysym.sym) {
        case SDLK_w:
        direction = DIR_UP;
        break;
        
        case SDLK_a:
        direction = DIR_LEFT;
        break;
        
        case SDLK_s:
        direction = DIR_DOWN;
        break;
        
        case SDLK_d:
        direction = DIR_RIGHT;
        break;
    }
    net_sendMove(sock, playerId, direction);
}

void game_showMainWindow(
    Player* me, sockets_t* sock, struct sockaddr_in* serverAddress) {
        
    SDL_Window* window = NULL;
    SDL_Surface* surface = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Rect camera = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    int mapWidth, mapHeight, tileCount;
    int levelWidth;
    Tile* tileSet; //Tile masīvs
    WTexture tileTexture;
    WTexture playerTexture;
    
    //Visi spēlētāju dati tiek glabāti heštabulā, kur atslēga ir spēlētāja id
    struct hashmap hm_players;
    
    //Init SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
		exit(1);
    }
    
    //Uzstāda texture filtering = linear
    if(!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
        printf( "Warning: Linear texture filtering not enabled!\n");
    }
    
    //Izveido logu
    window = SDL_CreateWindow(
        "Pacman",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    
    if (window == NULL) {
        printf("Window could not be created! SDL Error: %s\n", SDL_GetError() );
        exit(1);
    }
    
    //Izveido renderer
    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if(renderer == NULL) {
        printf( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
        exit(1);
    }
    
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    
    //Init SDL_image
    int imgFlags = IMG_INIT_PNG;
    if(!(IMG_Init(imgFlags) & imgFlags)) {
        printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
        exit(1);
    }
    
    wtexture_fromFile(&tileTexture, renderer, "tiles.png");
    wtexture_fromFile(&playerTexture, renderer, "players.png");
    
    tile_init();
    player_init();
    
    //Hashmap izmērs ir maksimālas spēlētāju skaits, kas ir vienāds ar spēlētāju
    //  skaitu, ko serveris var nosūtīt
    hashmap_init(&hm_players, hashmap_hash, hashmap_compare_keys, MAX_PACK_SIZE / OSIZE_PLAYER);
    
    //Uzzīmē logu
    surface = SDL_GetWindowSurface(window);
    SDL_FillRect( surface, NULL, SDL_MapRGB( surface->format, 0xFF, 0xFF, 0xFF ) );
    SDL_UpdateWindowSurface( window );
    
    //Gaida spēlēs sākumu no servera
    waitForStart(sock, me, &mapWidth, &mapHeight);
    tileCount = mapWidth * mapHeight;
    levelWidth = mapWidth * TILE_SPRITE_SIZE;
    tileSet = calloc(tileCount, sizeof(Tile));
    if (tileSet == NULL) {
        Die(ERR_MALLOC);
    }
    
    SDL_SetWindowSize(window, levelWidth, mapHeight * TILE_SPRITE_SIZE);
    
    bool quit = false;
    SDL_Event e;
    
    
    
    //Start thread
    pthread_t tcpThread;
    pthread_t udpThread;
    thread_args_t threadArgs = {
        .sockets = *sock,
        .tileCount = tileCount,
        .levelWidth = levelWidth,
        .me = me,
        .tiles = tileSet,
        .hm_players = &hm_players
    };
    
    pthread_create(&udpThread, NULL, thread_handleUdpPackets, &threadArgs);
    
    //Main loop
    while (!quit) {
        
        //Event loop
        while( SDL_PollEvent( &e ) != 0 ) {
            switch (e.type) {
                case SDL_QUIT:
                quit = true;
                break;
                
                case SDL_KEYDOWN:
                game_handleInput(sock->udp, me->id, &e.key);
                break;
            }
        }
        
        SDL_RenderClear(renderer);
        
        //Renderē tiles (pagaidām tikai atmiņā, nevis uz ekrāna)
        for (int i = 0; i < tileCount; i++) {
            tile_render(&tileSet[i], renderer, &camera, &tileTexture);
        }
        
        //Renderē spēlētājus atmiņā
        void* iter; //Hashmap iterators
        Player* pl;
        for (iter = hashmap_iter(&hm_players); iter; iter = hashmap_iter_next(&hm_players, iter)) {
            pl = hashmap_int_iter_get_data(iter);
            player_render(pl, &camera, &playerTexture, renderer);
        }
        player_render(me, &camera, &playerTexture, renderer);
        
        //Renderē visu uz ekrāna
        SDL_RenderPresent (renderer);
    }
    
    //Cleanup
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    free(tileSet);
    hashmap_destroy(&hm_players);
}

void* thread_handleUdpPackets(void* arg) {
    thread_args_t* convertedArgs = (thread_args_t*)arg;
    
    Player* playerPtr;
    int32_t playerCount;
    char packType;
    char* currentByte;
    char* pack = allocPack(MAX_PACK_SIZE);
    
    while (1) {
        safeRecv(convertedArgs->sockets.udp, pack, MAX_PACK_SIZE, 0);
        
        switch (pack[0]) {
        //Saņemta MAP pakete
        case PTYPE_MAP:
            printf("MAP\n");
            //X un Y nobīdes kartē
            int x = 0, y = 0;
            for (int i = 0; i < convertedArgs->tileCount; i++) {
                tile_new(&convertedArgs->tiles[i], x, y, pack[i+1]);
                
                x += TILE_SPRITE_SIZE;
                
                //"Pārlec" uz jaunu rindu
                if (x >= convertedArgs->levelWidth) {
                    x = 0;
                    y += TILE_SPRITE_SIZE;
                }
            }
            break;
        
            
        //Saņemta PLAYERS pakete
        case PTYPE_PLAYERS:
            //Paketes otrais līdz piektais baits nosaka spēlētāju skaitu
            playerCount = batoi(&pack[1]);
            
            //Kursors, kurš nosaka vietu, līdz kurai pakete nolasīta
            currentByte = &pack[5];
            
            for (int i = 0; i < playerCount; i++) {
                bool newPlayer = false;
                
                int id = batoi(currentByte);
                currentByte += 4;
                
                if (id == convertedArgs->me->id) {
                    playerPtr = convertedArgs->me;
                } else {
                    playerPtr = hashmap_int_get(convertedArgs->hm_players, &id);
                    
                    //Spēlētājs netika atrasts, tātad jāveido jauns
                    if (playerPtr == NULL) {
                        playerPtr = calloc(1, sizeof(Player));
                        if (playerPtr == NULL) {
                            Die(ERR_MALLOC);
                        }
                        newPlayer = true;
                    }
                }
                
                //Atjaunojam spēlētāja informāciju
                playerPtr->x = batof(currentByte);
                currentByte += 4;
                
                playerPtr->y = batof(currentByte);
                currentByte += 4;
                
                playerPtr->state = *currentByte;
                currentByte++;
                
                playerPtr->type = *currentByte;
                currentByte++;
                
                if (newPlayer) {
                    hashmap_int_put(convertedArgs->hm_players, &id, playerPtr);
                }
            }
            break;
        
            
        //Saņemta SCORE pakete
        case PTYPE_SCORE:
            //nulltais baits ir tipam, tāpēc sākam lasīt no pirmā
            currentByte = &pack[1];
            
            playerCount = batoi(currentByte);
            currentByte += 4;
            
            for (int i = 0; i < playerCount; i++) {
                int32_t score = batoi(currentByte);
                currentByte += 4;
                
                int32_t id = batoi(currentByte);
                currentByte += 4;
                
                
                if (id ==convertedArgs->me->id) {
                    convertedArgs->me->score = score;
                } else {
                    playerPtr = hashmap_int_get(convertedArgs->hm_players, &id);
                    if (playerPtr != NULL) {
                        playerPtr->score = score;
                    }
                }
            }
            
            break;
        }
    }
}

void* thread_handleTcpPackets(void* arg) {
    thread_args_t* convertedArgs = (thread_args_t*)arg;
}