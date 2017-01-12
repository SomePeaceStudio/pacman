#ifndef GAME_H
#define GAME_H

#include <netinet/in.h>
#include <SDL2/SDL_image.h>

#include "player.h"
#include "../shared/shared.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

void game_showMainWindow(
    Player* me, sockets_t* sock, struct sockaddr_in* serverAddress);

#endif //GAME_H