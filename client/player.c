#include "player.h"
#include "tile.h"
#include "../shared/shared.h"

//Privātie mainīgie
static SDL_Rect textureClips[TOTAL_PLAYER_SPRITES];

void player_render(Player* player, SDL_Rect* camera, WTexture* wt, SDL_Renderer* renderer) {
    wtexture_render(
        wt,
        renderer,
        player->x - camera->x,
        player->y - camera->y,
        &textureClips[player->type],
        0,
        NULL,
        SDL_FLIP_NONE
    );
}

//Inicializē sprite robežas failā res/players.png
void player_init() {
    //Pacman sprite
    textureClips[PLTYPE_PACMAN].x = 0;
    textureClips[PLTYPE_PACMAN].y = 0;
    textureClips[PLTYPE_PACMAN].w = PLAYER_SPRITE_SIZE;
    textureClips[PLTYPE_PACMAN].h = PLAYER_SPRITE_SIZE;
    
    //Ghost sprite
    textureClips[PLTYPE_GHOST].x = PLAYER_SPRITE_SIZE;
    textureClips[PLTYPE_GHOST].y = 0;
    textureClips[PLTYPE_GHOST].w = PLAYER_SPRITE_SIZE;
    textureClips[PLTYPE_GHOST].h = PLAYER_SPRITE_SIZE;
}

void player_setCamera(
    Player* player,
    SDL_Rect* camera,
    int screenWidth,
    int screenheight,
    int levelWidth,
    int levelHeight) {
    
    camera->x = (player->x + PLAYER_SPRITE_SIZE/2) - screenWidth/2;
    camera->y = (player->y + PLAYER_SPRITE_SIZE/2) - screenheight/2;
    
    camera->x = clipBoth(camera->x, 0, levelWidth - camera->w);
    camera->y = clipBoth(camera->y, 0, levelHeight - camera->h);
}