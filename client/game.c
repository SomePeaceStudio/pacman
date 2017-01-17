#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <pthread.h>

#include "game.h"
#include "tile.h"
#include "player.h"
#include "network.h"
#include "../shared/hashmap.h"

//72, jo serveris vairāk nesūtīs (1024 baitu buferī vairāk nesaiet)
#define PLAYER_COUNT_MAX 72

#define FONT_SIZE 15

//Nodefinē hashmap priekš int atslēgām un Player tipa datiem
HASHMAP_FUNCS_DECLARE(int, int32_t, Player);
HASHMAP_FUNCS_CREATE(int, int32_t, Player);

void waitForStart();
//Atgriež tekstu ar spēlētāju punktiem
char** getScoreText(struct hashmap* hm_players, Player* me);
//Uzzīmē spēlētāju nikus un punktu skaitus uz ekrāna
void renderPlayerScores(SDL_Renderer* renderer, SDL_Color color, TTF_Font* font, char** scores, size_t lineCount);
size_t getDigitCount(int32_t i); //Atgriež ciparu skaitu skaitlī
void game_handleInput(int sock, int playerId, SDL_KeyboardEvent* event);
void game_showMainWindow(Player* me, sockets_t* sock, struct sockaddr_in* serverAddress);


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
    char direction = DIR_NONE;
    switch (event->keysym.sym) {
        case SDLK_w:
        case SDLK_UP:
        direction = DIR_UP;
        break;
        
        case SDLK_a:
        case SDLK_LEFT:
        direction = DIR_LEFT;
        break;
        
        case SDLK_s:
        case SDLK_DOWN:
        direction = DIR_DOWN;
        break;
        
        case SDLK_d:
        case SDLK_RIGHT:
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
    
    memset(&tileTexture, 0, sizeof(WTexture));
    memset(&playerTexture, 0, sizeof(WTexture));
    
    //Visi spēlētāju dati tiek glabāti heštabulā, kur atslēga ir spēlētāja id
    struct hashmap hm_players;
    
    //Init SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
		exit(1);
    }
    
    if(TTF_Init() == -1){
        printf( "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError() );
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

    //Pavedieni, kas saņems attiecīgi TCP un UDP paketes
    pthread_t tcpThread;
    pthread_t udpThread;
    
    //Info, kas japadod TCP un UDP pavedieniem
    thread_args_t threadArgs = {
        .sockets = *sock,
        .tileCount = tileCount,
        .levelWidth = levelWidth,
        .me = me,
        .tiles = tileSet,
        .hm_players = &hm_players
    };
    
    //Izveidojam pavedienus
    pthread_create(&udpThread, NULL, net_handleUdpPackets, &threadArgs);
    pthread_create(&tcpThread, NULL, net_handleTcpPackets, &threadArgs);
    
    bool quit = false;
    bool showScores = false;
    SDL_Event e;
    
    TTF_Font* font = TTF_OpenFont("font.ttf", FONT_SIZE);
    
    if (font == NULL) {
        printf("font == NULL\n");
        exit(1);
    }
    
    SDL_Color color = {.r = 180, .g = 180, .b = 180, .a = 100};
    SDL_Color bgColor = {.r = 255, .g = 255, .b = 255, .a = 100};
    
    //Main loop
    while (!quit) {
        
        //Event loop
        while( SDL_PollEvent( &e ) != 0 ) {
            switch (e.type) {
            case SDL_QUIT:
                quit = true;
                break;
            
                
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_TAB) {
                    showScores = true;
                } else {
                    game_handleInput(sock->udp, me->id, &e.key);
                }
                break;
                
                
            case SDL_KEYUP:
                if (e.key.keysym.sym == SDLK_TAB) {
                    showScores = false;
                }
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
        
        if (showScores) {
            char ** scores = getScoreText(&hm_players, me);
            renderPlayerScores(renderer, color, font, scores, hashmap_size(&hm_players) + 1);
        }
        
        //Renderē visu uz ekrāna
        SDL_RenderPresent (renderer);
    }
    
    //Cleanup
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    free(tileSet);
    hashmap_destroy(&hm_players);
    wtexture_free(&tileTexture);
    wtexture_free(&playerTexture);
    SDL_Quit();
    TTF_Quit();
}

char** getScoreText(struct hashmap* hm_players, Player* me) {
    size_t playerCount = hashmap_size(hm_players) + 1; //+1 priekš "me"
    char** textLines;
    
    textLines = malloc(playerCount * sizeof(char*));
    
    //Iterē pāri visiem spēlētājiem (ieskaitot "me")
    void* iter;
    int i;
    for (iter = hashmap_iter(hm_players), i = 0; i < playerCount; iter = hashmap_iter_next(hm_players, iter), i++) {
        Player* pl = hashmap_int_iter_get_data(iter);
        
        //Ja pl ir null, tad visi pārējie spēlētāji jau ir apskatīti
        //Jāpievieno vel tikai savs rezultāts
        if (pl == NULL) {
            pl = me;
        }
        
        //Ja spēlētājam nav nika, tad izveidojam to formā "[ID-13]"
        if (pl->nick == NULL) {
            size_t nickLength = 6 + getDigitCount(pl->id);
            pl->nick = malloc (nickLength);
            if (pl->nick == NULL) {
                Die(ERR_MALLOC);
            }
            snprintf(pl->nick, nickLength, "[ID-%d]", pl->id);
        }
        
        //Formāts: "niks: 123\0"
        size_t lineLength = strlen(pl->nick) + 2 + getDigitCount(pl->score) + 1;
        
        textLines[i] = malloc(lineLength);
        snprintf(textLines[i], lineLength, "%s: %d", pl->nick, pl->score);
    }
    
    return textLines;
}

void renderPlayerScores(SDL_Renderer* renderer, SDL_Color color, TTF_Font* font, char** scores, size_t lineCount) {
    //Vieta uz ekrāna, kur rezultāts tiks parādīts
    SDL_Rect targetRect = {.x = 8, .y = 0};
    
    //hashmap izmēram + 1, jo "me" nav iekļauts hashmap'ā
    for (int i = 0; i < lineCount; i++) {
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, scores[i], color);
        targetRect.w = textSurface->w;
        targetRect.h = textSurface->h;
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_RenderCopy(renderer, texture, NULL, &targetRect);
        targetRect.y += textSurface->h;
        SDL_FreeSurface(textSurface);
        
    }
}

//Darbojas ātri, jo nav lieki aprēķini
size_t getDigitCount(int32_t i) {
    i = abs(i);
    if (i < 10) return 1;
    if (i < 100) return 2;
    if (i < 1000) return 3;
    if (i < 10000) return 4;
    if (i < 100000) return 5;
    if (i < 1000000) return 6;
    if (i < 10000000) return 7;
    if (i < 100000000) return 8;
    if (i < 1000000000) return 9;
    if (i < 10000000000) return 10; //32 bitos nebūs vairāk par 10 cipariem
}