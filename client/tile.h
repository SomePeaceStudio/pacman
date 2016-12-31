#ifndef TILE_H
#define TILE_H

#include <SDL2/SDL.h>

//Rūtiņas platums / garums
#define TILE_SIZE 10

typedef struct {
    char type;
    SDL_Rect box;
} Tile;

Tile* tile_new(int x, int y, char type);
void tile_render(Tile* tile, SDL_Rect* camera);

#endif //TILE_H