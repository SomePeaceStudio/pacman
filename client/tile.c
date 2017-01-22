#include <stdbool.h>

#include "tile.h"
#include "../shared/shared.h"

static bool checkCollision(SDL_Rect a, SDL_Rect b);

static SDL_Rect textureClips[TOTAL_TILE_SPRITES];

void tile_new(Tile* tile, int x, int y, char type) {
    if (tile == NULL) {
        return;
    }
    
    tile->box.x = x;
    tile->box.y = y;
    tile->box.w = tile->box.h = TILE_SPRITE_SIZE;
    tile->type = type;
}

void tile_render(Tile* tile, SDL_Renderer* renderer, SDL_Rect* camera, WTexture* tileTexture) {
    if (checkCollision(*camera, tile->box)) {
        wtexture_render_2(
            tileTexture,
            renderer,
            tile->box.x - camera->x,
            tile->box.y - camera->y,
            &textureClips[tile->type],
            0,
            NULL,
            SDL_FLIP_NONE
        );
    }
}

//Inicializē "sprites" robežas failā res/tiles.png
void tile_init() {
    textureClips[MTYPE_EMPTY].x = 0;
    textureClips[MTYPE_EMPTY].y = 0;
    textureClips[MTYPE_EMPTY].w = TILE_SPRITE_SIZE;
    textureClips[MTYPE_EMPTY].h = TILE_SPRITE_SIZE;
    
    textureClips[MTYPE_DOT].x = TILE_SPRITE_SIZE;
    textureClips[MTYPE_DOT].y = 0;
    textureClips[MTYPE_DOT].w = TILE_SPRITE_SIZE;
    textureClips[MTYPE_DOT].h = TILE_SPRITE_SIZE;
    
    textureClips[MTYPE_WALL].x = TILE_SPRITE_SIZE * 2;
    textureClips[MTYPE_WALL].y = 0;
    textureClips[MTYPE_WALL].w = TILE_SPRITE_SIZE;
    textureClips[MTYPE_WALL].h = TILE_SPRITE_SIZE;
    
    textureClips[MTYPE_POWER].x = 0;
    textureClips[MTYPE_POWER].y = TILE_SPRITE_SIZE;
    textureClips[MTYPE_POWER].w = TILE_SPRITE_SIZE;
    textureClips[MTYPE_POWER].h = TILE_SPRITE_SIZE;
    
    textureClips[MTYPE_INVINCIBILITY].x = TILE_SPRITE_SIZE;
    textureClips[MTYPE_INVINCIBILITY].y = TILE_SPRITE_SIZE;
    textureClips[MTYPE_INVINCIBILITY].w = TILE_SPRITE_SIZE;
    textureClips[MTYPE_INVINCIBILITY].h = TILE_SPRITE_SIZE;
    
    textureClips[MTYPE_SCORE].x = TILE_SPRITE_SIZE * 2;
    textureClips[MTYPE_SCORE].y = TILE_SPRITE_SIZE;
    textureClips[MTYPE_SCORE].w = TILE_SPRITE_SIZE;
    textureClips[MTYPE_SCORE].h = TILE_SPRITE_SIZE;
}

static bool checkCollision(SDL_Rect a, SDL_Rect b) {
    //Četrstūru malas
    int leftA, leftB;
    int rightA, rightB;
    int topA, topB;
    int bottomA, bottomB;

    //Aprēķina a malas
    leftA = a.x;
    rightA = a.x + a.w;
    topA = a.y;
    bottomA = a.y + a.h;

    //Aprēķina b malas
    leftB = b.x;
    rightB = b.x + b.w;
    topB = b.y;
    bottomB = b.y + b.h;

    //Ja kāda no A malām ir ārpus B
    if( bottomA <= topB )
        return false;

    if( topA >= bottomB )
        return false;

    if( rightA <= leftB )
        return false;

    if( leftA >= rightB )
        return false;

    //Ja neviena no a malām neatrodas ārpus b
    return true;
}