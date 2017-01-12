#include "player.h"
#include "tile.h"
#include "../shared/shared.h"

//Privātie mainīgie
static SDL_Rect textureClips[TOTAL_PLAYER_SPRITES];

void player_render(Player* player, SDL_Rect* camera, WTexture* wt, SDL_Renderer* renderer) {
    wtexture_render(
        wt,
        renderer,
        player->x * TILE_SPRITE_SIZE,
        player->y * TILE_SPRITE_SIZE,
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