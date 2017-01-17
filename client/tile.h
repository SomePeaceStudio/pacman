#ifndef TILE_H
#define TILE_H

#include <SDL2/SDL.h>

#include "wtexture.h"

//Rūtiņas platums / garums
#define TILE_SPRITE_SIZE 25
//Dažādo "šunu" skaits
#define TOTAL_TILE_SPRITES 6

typedef struct {
    char type;
    SDL_Rect box;
} Tile;

//Šis jāizsauc pirms jebkādas tile renderēšanas
void tile_init();
void tile_new(Tile* tile, int x, int y, char type);
void tile_render(Tile* tile, SDL_Renderer* renderer, SDL_Rect* camera, WTexture* tileTexture);

#endif //TILE_H