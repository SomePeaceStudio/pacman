#ifndef TEXTURE_H
#define TEXTURE_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <SDL2/SDL_ttf.h>


//Texture wrapper struktūra
typedef struct {
    int width;
    int height;
    SDL_Texture* texture;
} WTexture;

void wtexture_fromFile(WTexture* wtexture, SDL_Renderer* renderer, const char* path);
bool wtexture_fromText(WTexture* wtexture, SDL_Renderer* renderer, TTF_Font* font, SDL_Color textColor, SDL_Color bgColor, const char* text);
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