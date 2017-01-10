#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>

#include "game.h"
#include "tile.h"

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

void game_showMainWindow(
    Player* player, sockets_t* sock, struct sockaddr_in* serverAddress) {
        
    SDL_Window* window = NULL;
    SDL_Surface* surface = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Rect camera = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    int mapWidth, mapHeight, tileCount;
    int levelWidth;
    Tile* tileSet; //Tile masīvs
    WTexture tileTexture;
    SDL_Rect tileClips[TOTAL_TILE_SPRITES];
    
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

    //Uzzīmē logu
    surface = SDL_GetWindowSurface(window);
    SDL_FillRect( surface, NULL, SDL_MapRGB( surface->format, 0xFF, 0xFF, 0xFF ) );
    SDL_UpdateWindowSurface( window );
    
    //Gaida spēlēs sākumu no servera
    waitForStart(sock, player, &mapWidth, &mapHeight);
    tileCount = mapWidth * mapHeight;
    levelWidth = mapWidth * TILE_SIZE;
    tileSet = calloc(tileCount, sizeof(Tile));
    if (tileSet == NULL) {
        Die(ERR_MALLOC);
    }
    
    SDL_SetWindowSize(window, levelWidth, mapHeight * TILE_SIZE);
    
    char packType;
    char* pack = allocPack(1024);
    
    bool quit = false;
    SDL_Event e;
    
    tileClips[MTYPE_EMPTY].x = 0;
    tileClips[MTYPE_EMPTY].y = 0;
    tileClips[MTYPE_EMPTY].w = TILE_SIZE;
    tileClips[MTYPE_EMPTY].h = TILE_SIZE;
    
    tileClips[MTYPE_DOT].x = 50;
    tileClips[MTYPE_DOT].y = 0;
    tileClips[MTYPE_DOT].w = TILE_SIZE;
    tileClips[MTYPE_DOT].h = TILE_SIZE;
    
    tileClips[MTYPE_WALL].x = 100;
    tileClips[MTYPE_WALL].y = 0;
    tileClips[MTYPE_WALL].w = TILE_SIZE;
    tileClips[MTYPE_WALL].h = TILE_SIZE;
    
    tileClips[MTYPE_POWER].x = 0;
    tileClips[MTYPE_POWER].y = 50;
    tileClips[MTYPE_POWER].w = TILE_SIZE;
    tileClips[MTYPE_POWER].h = TILE_SIZE;
    
    tileClips[MTYPE_INVINCIBILITY].x = 50;
    tileClips[MTYPE_INVINCIBILITY].y = 50;
    tileClips[MTYPE_INVINCIBILITY].w = TILE_SIZE;
    tileClips[MTYPE_INVINCIBILITY].h = TILE_SIZE;
    
    tileClips[MTYPE_SCORE].x = 100;
    tileClips[MTYPE_SCORE].y = 50;
    tileClips[MTYPE_SCORE].w = TILE_SIZE;
    tileClips[MTYPE_SCORE].h = TILE_SIZE;
    
    //Main loop
    while (!quit) {
        
        //Apstrādā notikumus (events)
        while( SDL_PollEvent( &e ) != 0 ) {
            if(e.type == SDL_QUIT) {
                quit = true;
            }
        }
        
        int received = safeRecv(sock->udp, pack, 1024, 0);
        int32_t playerCount;
        switch (pack[0]) {
            case PTYPE_MAP:
            printf("MAP\n");
            //X un Y nobīdes kartē
            int x = 0, y = 0;
            for (int i = 0; i < tileCount; i++) {
                tile_new(&tileSet[i], x, y, pack[i+1]);
                
                x += TILE_SIZE;
                
                //"Pārlec" uz jaunu rindu
                if (x >= levelWidth) {
                    x = 0;
                    y += TILE_SIZE;
                }
            }
            
            SDL_SetRenderDrawColor( renderer, 0xFF, 0xFF, 0xFF, 0xFF );
            SDL_RenderClear( renderer );
            
            //Uzzīmē karti
            for (int i = 0; i < tileCount; i++) {
                tile_render(&tileSet[i], renderer, &camera, &tileTexture, tileClips);
            }
            SDL_RenderPresent (renderer);
            break;
            
            
            case PTYPE_PLAYERS:
            printf("PLAYERS\n");
            //Paketes otrais līdz piektais baits nosaka spēlētāju skaitu
            playerCount = batoi(&pack[1]);
            printf("Player count: %d\n", playerCount);
            const size_t objSize = 14;
            printPacket(pack, 5 + playerCount * objSize);
            
            //Darbojas, kā kursors, kurš glabā vietu, līdz kurai pakete nolasīta
            char* currentByte = &pack[5];
            for (int i = 0; i < playerCount; i++) {
                int id = batoi(currentByte);
                currentByte += 4;
                
                float x = batof(currentByte);
                currentByte += 4;
                
                float y = batof(currentByte);
                currentByte += 4;
                
                char state = *currentByte;
                currentByte++;
                
                char type = *currentByte;
                currentByte++;
                
                printf("Player %d is at %f %f state: %x, type: %x\n", id, x, y, state, type);
            }
            
            break;
            
            case PTYPE_SCORE:
            // printf("SCORE\n");
            break;
        }
    }
    
    //Cleanup
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    free(tileSet);
}