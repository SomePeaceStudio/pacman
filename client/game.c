#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>

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
    
    hashmap_init(&hm_players, hashmap_hash, hashmap_compare_keys, PLAYER_COUNT_MAX);
    
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
    
    char packType;
    char* pack = allocPack(1024);
    
    bool quit = false;
    SDL_Event e;
    
    //Main loop
    while (!quit) {
        
        //Apstrādā notikumus (events)
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
        
        int received = safeRecv(sock->udp, pack, 1024, 0);
        
        int32_t playerCount;
        unsigned char* currentByte;
        
        switch (pack[0]) {
            case PTYPE_MAP:
            printf("MAP\n");
            //X un Y nobīdes kartē
            int x = 0, y = 0;
            for (int i = 0; i < tileCount; i++) {
                tile_new(&tileSet[i], x, y, pack[i+1]);
                
                x += TILE_SPRITE_SIZE;
                
                //"Pārlec" uz jaunu rindu
                if (x >= levelWidth) {
                    x = 0;
                    y += TILE_SPRITE_SIZE;
                }
            }
            
            SDL_SetRenderDrawColor( renderer, 0xFF, 0xFF, 0xFF, 0xFF );
            SDL_RenderClear( renderer );
            
            //Uzzīmē karti
            for (int i = 0; i < tileCount; i++) {
                tile_render(&tileSet[i], renderer, &camera, &tileTexture);
            }
            SDL_RenderPresent (renderer);
            break;
            
            
            case PTYPE_PLAYERS:
            //Paketes otrais līdz piektais baits nosaka spēlētāju skaitu
            playerCount = batoi(&pack[1]);
            
            //Darbojas, kā kursors, kurš glabā vietu, līdz kurai pakete nolasīta
            currentByte = &pack[5];
            
            for (int i = 0; i < playerCount; i++) {
                Player* player;
                bool newPlayer = false;
                
                int id = batoi(currentByte);
                currentByte += 4;
                
                if (id == me->id) {
                    player = me;
                } else {
                    player = hashmap_int_get(&hm_players, &id);
                    
                    //Spēlētājs netika atrasts (spēlei pievienojies jauns)
                    if (player == NULL) {
                        player = calloc(1, sizeof(player));
                        if (player == NULL) {
                            Die(ERR_MALLOC);
                        }
                        newPlayer = true;
                    }
                }
                
                //Atjaunojam spēlētāja informāciju
                player->x = batof(currentByte);
                currentByte += 4;
                
                player->y = batof(currentByte);
                currentByte += 4;
                
                player->state = *currentByte;
                currentByte++;
                
                player->type = *currentByte;
                currentByte++;
                
                if (newPlayer) {
                    hashmap_int_put(&hm_players, &id, player);
                }
                
                player_render(player, &camera, &playerTexture, renderer);
                SDL_RenderPresent(renderer);
            }
            
            break;
            
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
                
                if (id == me->id) {
                    printf("My score: %d\n", score);
                } else {
                    Player* p = hashmap_int_get(&hm_players, &id);
                    if (p == NULL) continue;
                    printf("Player with id %d has score: %d\n", p->id, score);
                }
            }
            
            break;
        }
    }
    
    //Cleanup
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    free(tileSet);
    hashmap_destroy(&hm_players);
}