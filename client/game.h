#ifndef GAME_H
#define GAME_H

#include <netinet/in.h>
#include <SDL2/SDL_image.h>

#include "player.h"
#include "../shared/shared.h"

typedef enum { GS_LOG_OUT, GS_QUIT } GameStatus;

GameStatus game_showMainWindow(
    Player* me, sockets_t* sock, struct sockaddr_in* serverAddress);

#endif //GAME_H