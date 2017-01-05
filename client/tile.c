#include <stdbool.h>

#include "tile.h"
#include "../shared/shared.h"

Tile* tile_new(int x, int y, char type) {
    Tile* tile = malloc(sizeof(Tile));
    if (tile == NULL) {
        Die(ERR_MALLOC);
    }
    
    tile->box.x = x;
    tile->box.y = y;
    tile->box.w = tile->box.h = TILE_SIZE;
    tile->type = type;
    
    return tile;
}

bool checkCollision(SDL_Rect a, SDL_Rect b) {
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

    //If any of the sides from A are outside of B
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

void tile_render(Tile* tile, SDL_Renderer* renderer, SDL_Rect* camera, WTexture* tileTexture, SDL_Rect* tileClips) {
    if (checkCollision(*camera, tile->box)) {
        wtexture_render(
            tileTexture,
            renderer,
            tile->box.x - camera->x,
            tile->box.y - camera->y,
            &tileClips[tile->type],
            0,
            NULL,
            SDL_FLIP_NONE
        );
    }
}