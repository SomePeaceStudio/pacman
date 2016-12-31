#ifndef TEXTURE_H
#define TEXTURE_H

#include <SDL2/SDL.h>

typedef struct {
    int width;
    int height;
    SDL_Texture* texture;
} Texture;

Texture* texture_fromFile(SDL_Renderer* rederer, const char* path);
void texture_render(int x, int y, SDL_Rect* clip, SDL_Renderer* renderer);
void texture_free(Texture* tx);


#endif //TEXTURE_H