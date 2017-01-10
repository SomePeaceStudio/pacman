#ifndef TILE_H
#define TILE_H

#include <SDL2/SDL.h>

#include "wtexture.h"

//Rūtiņas platums / garums
#define TILE_SIZE 50
#define TOTAL_TILE_SPRITES 6

typedef struct {
    char type;
    SDL_Rect box;
} Tile;

void tile_new(Tile* tile, int x, int y, char type);
void tile_render(Tile* tile, SDL_Renderer* renderer, SDL_Rect* camera, WTexture* tileTexture, SDL_Rect* tileClips);

#endif //TILE_H