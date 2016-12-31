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

void tile_render(Tile* tile, SDL_Rect* camera) {
    
}