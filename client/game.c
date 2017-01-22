#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <pthread.h>

#include "game.h"
#include "tile.h"
#include "player.h"
#include "network.h"
#include "string.h"
#include "../shared/hashmap.h"

//72, jo serveris vairāk nesūtīs (1024 baitu buferī vairāk nesaiet)
#define PLAYER_COUNT_MAX 72

#define FONT_SIZE 15

//Nodefinē hashmap priekš int atslēgām un Player tipa datiem
HASHMAP_FUNCS_DECLARE(int, int32_t, Player);
HASHMAP_FUNCS_CREATE(int, int32_t, Player);

int min(int a, int b) {
    return (a > b) ? b : a;
}
    
void waitForStart();
//Atgriež tekstu ar spēlētāju punktiem
char** getScoreText(struct hashmap* hm_players, Player* me);
//Uzzīmē spēlētāju nikus un punktu skaitus uz ekrāna
void renderPlayerScores(SDL_Renderer* renderer, SDL_Color color, TTF_Font* font, char** scores, size_t lineCount);
//Uzzīmē spēles galveno izvēlni (kad nospiež ESC)
void renderPauseScreen(SDL_Renderer* renderer, TTF_Font* font, int windth, int height);
size_t getDigitCount(int32_t i); //Atgriež ciparu skaitu skaitlī
void game_handleInput(int sock, int playerId, SDL_KeyboardEvent* event);

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

GameStatus game_showMainWindow(
    Player* me, sockets_t* sock, struct sockaddr_in* serverAddress) {
        
    SDL_Window* window = NULL;
    SDL_Surface* surface = NULL;
    SDL_Renderer* renderer = NULL;
    //Domāts apciprpšanai
    SDL_Rect camera = { 0, 0, 500, 500 };
    int mapWidth, mapHeight, tileCount;
    int levelWidth, levelHeight;
    int windowWidth, windowHeight;
    Tile* tileSet; //Tile masīvs
    WTexture tileTexture;
    WTexture playerTexture;
    GameStatus returnStatus = GS_QUIT;
    
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
    
    //Izveido 500x500 px logu (izmēriem nav nozīmes, jo tie tapat mainīsies,
    //  kad no servera tiks saņemta karte)
    window = SDL_CreateWindow(
        "Pacman",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        500,
        500,
        SDL_WINDOW_RESIZABLE
    );
    
    if (window == NULL) {
        printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
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
    
    //Lai varētu zīmēt arī caurspīdīgumu (Alpha channel)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
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
    hashmap_init(&hm_players, hashmap_hash, hashmap_compare_keys, DEFAULT_PACK_SIZE / OSIZE_PLAYER);
    
    //Uzzīmē logu
    surface = SDL_GetWindowSurface(window);
    SDL_FillRect( surface, NULL, SDL_MapRGB( surface->format, 0x33, 0x33, 0x33 ) );
    SDL_UpdateWindowSurface( window );
    
    //Gaida spēlēs sākumu no servera
    waitForStart(sock, me, &mapWidth, &mapHeight);
    tileCount = mapWidth * mapHeight;
    tileSet = calloc(tileCount, sizeof(Tile));
    if (tileSet == NULL) {
        Die(ERR_MALLOC);
    }
    
    levelWidth = mapWidth * TILE_SPRITE_SIZE;
    levelHeight = mapHeight * TILE_SPRITE_SIZE;
    
    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
        printf("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
        exit (1);
    }
    
    //Ja līmenis ir gan par platu, gan par augstu, tad maksimizē logu,
    //  citādi, izstiepj vajadzīgajā virzienā
    if (levelWidth > dm.w && levelHeight > dm.h) {
        SDL_MaximizeWindow(window);
    } else {
        SDL_SetWindowSize(
            window,
            clipMax(levelWidth, dm.w),
            clipMax(levelHeight, dm.h)
        );
    }

    //Pavedieni, kas saņems attiecīgi TCP un UDP paketes
    pthread_t tcpThread;
    pthread_t udpThread;
    
    bool quit = false;
    
    //Info, kas japadod TCP un UDP pavedieniem
    thread_args_t threadArgs = {
        .sockets = *sock,
        .tileCount = tileCount,
        .levelWidth = &levelWidth,
        .levelHeight = &levelHeight,
        .windowWidth = &windowWidth,
        .windowHeight = &windowHeight,
        .camera = &camera,
        .me = me,
        .quit = &quit,
        .tiles = tileSet,
        .hm_players = &hm_players
    };
    
    //Uzstāda 1 sekundes timeout TCP un UDP socketiem, lai pavedieni negaidītu
    //  paketi mūžīgi. (Svarīgi biegās, kad jāatbrīvo resursi)
    struct timeval tv;
    tv.tv_sec = 1;   //1 sekunde
    tv.tv_usec = 0;  //Mikrosekundes jāinicializē uz 0
    setsockopt( 
        sock->tcp,
        SOL_SOCKET, SO_RCVTIMEO,
        (char *)&tv,
        sizeof(struct timeval)
    );
    setsockopt( 
        sock->udp,
        SOL_SOCKET, SO_RCVTIMEO,
        (char *)&tv,
        sizeof(struct timeval)
    );
    
    //Izveidojam pavedienus
    pthread_create(&udpThread, NULL, net_handleUdpPackets, &threadArgs);
    pthread_create(&tcpThread, NULL, net_handleTcpPackets, &threadArgs);
    
    //Fonts teksta renderēšanai
    TTF_Font* font = TTF_OpenFont("font.ttf", FONT_SIZE);
    if (font == NULL) {
        printf("font == NULL\n");
        exit(1);
    }
    
    //Atslēdz teksta ievadi sākumā
    SDL_StopTextInput();
    
    //Teksta krāsa
    SDL_Color color = {.r = 180, .g = 180, .b = 180, .a = 100};
    bool showScores = false;
    bool pause = false;
    SDL_Event e;
    
    string chatMessage;
    bool textInputActive = false;
    
    //Main loop
    while (!quit) {
        
        //Event loop
        while( SDL_PollEvent( &e ) != 0 ) {
            switch (e.type) {
            case SDL_QUIT:
                quit = true;
                break;
            
                
            case SDL_KEYDOWN:
                switch (e.key.keysym.sym) {
                case SDLK_TAB:
                    if (e.key.repeat == 0) {
                        showScores = true;
                    }
                    break;
                    
                case SDLK_SLASH:
                    if (e.key.repeat == 0) {
                        if (textInputActive) {
                            SDL_StopTextInput();
                            str_free(&chatMessage);
                        } else {
                            SDL_StartTextInput();
                            str_new(&chatMessage, NULL);
                        }
                        textInputActive = !textInputActive;
                    }
                    break;
                    
                case SDLK_BACKSPACE:
                    str_popBack(&chatMessage);
                    printf("Chat message: %s\n", chatMessage.buffer);
                    break;
                    
                case SDLK_ESCAPE:
                    if (e.key.repeat == 0) {
                        printf("pause\n");
                        if (textInputActive) {
                            textInputActive = false;
                        } else {
                            pause = !pause;
                        }
                    }
                    break;
                    
                //Pause: Continue
                case SDLK_1:
                    if (e.key.repeat == 0) {
                        if (pause) {
                            pause = false;
                        }
                    }
                    break;
                
                //Pause: Log out
                case SDLK_2:
                    if (e.key.repeat == 0) {
                        if (pause) {
                            printf("Sending QUIT\n");
                            net_sendQuit(sock->tcp, me->id);
                            printf("Sent QUIT\n");
                            returnStatus = GS_LOG_OUT;
                            quit = true;
                        }
                    }
                    break;
                    
                //Pause: Quit
                case SDLK_3:
                    if (e.key.repeat == 0) {
                        if (pause) {
                            quit = true;
                        }
                    }
                    break;
                    
                default:
                    if (e.key.repeat == 0) {
                        game_handleInput(sock->udp, me->id, &e.key);
                    }
                    break;    
                }
                break;
                
                
            case SDL_KEYUP:
                if (e.key.keysym.sym == SDLK_TAB) {
                    showScores = false;
                }
                break;
                
                
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    camera.w = windowWidth = e.window.data1;
                    camera.h = windowHeight = e.window.data2;
                }
                break;
                
            
            case SDL_TEXTINPUT:
                str_appendString(&chatMessage, e.text.text);
                printf("Chat message: %s\n", chatMessage.buffer);
                break;
            }
        }
        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
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
        
        if (textInputActive) {
            //Uzzīmē pelēku fonu, kur ievadīt tekstu
            SDL_Rect chatBox = {.w = windowWidth, .h = 30, .x = 0, .y = windowHeight - 30};
            SDL_SetRenderDrawColor(renderer, 70, 70, 70, 128);
            SDL_RenderFillRect(renderer, &chatBox);
            
            
            SDL_Color textColor = {.r = 230, .g = 230, .b = 230, .a = 230};
            SDL_Surface* textSurface = TTF_RenderText_Solid(font, chatMessage.buffer, textColor);
            
            if (textSurface != NULL) {
                SDL_Rect targetRect = {
                    .w = min(windowWidth, textSurface->w),
                    .h = textSurface->h,
                    .x = 0,
                    .y = windowHeight - textSurface->h
                };
                
                SDL_Rect clipRect = {
                    .w = windowWidth,
                    .h = 30,
                    .x = textSurface->w - windowWidth,
                    .y = 0
                };
                
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, textSurface);
                SDL_RenderCopy(renderer, texture, &clipRect, &targetRect);
                SDL_FreeSurface(textSurface);
            }
        }
        
        if (pause) {
            renderPauseScreen(renderer, font, windowWidth, windowHeight);
        }
        
        //Renderē visu uz ekrāna
        SDL_RenderPresent (renderer);
    }
    
    //Pagaidām, kamēr abi tīkla pavedien biedz darbu
    pthread_join(tcpThread, NULL);
    pthread_join(udpThread, NULL);
    
    //Cleanup
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    free(tileSet);
    hashmap_destroy(&hm_players);
    wtexture_free(&tileTexture);
    wtexture_free(&playerTexture);
    SDL_Quit();
    TTF_Quit();
    return returnStatus;
}

char** getScoreText(struct hashmap* hm_players, Player* me) {
    //+1 priekš "me", jo tas nav hashmap'ā
    size_t playerCount = hashmap_size(hm_players) + 1;
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
    SDL_Rect targetRect = {.x = 8, .y = 8};
    
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

void renderPauseScreen(SDL_Renderer* renderer, TTF_Font* font, int windth, int height) {
    static WTexture wtResume;
    static WTexture wtLogOut;
    static WTexture wtQuit;
    
    SDL_Rect chatBox = {.w = windth, .h = height, .x = 0, .y = 0};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_RenderFillRect(renderer, &chatBox);
    
    
    SDL_Color textColor = {.r = 255, .g = 255, .b = 255};
    SDL_Color bgColor = {.r = 0, .g = 0, .b = 0};
    
    if (wtResume.texture == NULL) {
        printf("Initializing\n");
        wtexture_fromText(&wtResume, renderer, font, textColor, bgColor, "1: Resume");
        wtexture_fromText(&wtLogOut, renderer, font, textColor, bgColor, "2: Log out");
        wtexture_fromText(&wtQuit, renderer, font, textColor, bgColor, "3: Quit");
    }
    
    wtexture_render(&wtResume, renderer, 10, 10, NULL);
    wtexture_render(&wtLogOut, renderer, 10, 10 + wtResume.height, NULL);
    wtexture_render(&wtQuit, renderer, 10, 10 + wtResume.height * 2, NULL);
}

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