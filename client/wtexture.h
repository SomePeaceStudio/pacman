#ifndef TEXTURE_H
#define TEXTURE_H

#include <SDL2/SDL.h>

//Texture wrapper struktūra
typedef struct {
    int width;
    int height;
    SDL_Texture* texture;
} WTexture;

WTexture* wtexture_fromFile(SDL_Renderer* rederer, const char* path);
void wtexture_free(WTexture* wtexture);
void wtexture_render(
    WTexture* wtexture,
    SDL_Renderer* renderer,
    int x,
    int y,
    SDL_Rect* clip,
    double angle,
    SDL_Point* center,
    SDL_RendererFlip flip
);


#endif //TEXTURE_H