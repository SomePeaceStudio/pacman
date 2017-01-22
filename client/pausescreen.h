#ifndef PAUSE_H
#define PAUSE_H

#include "wtexture.h"

typedef struct {
    WTexture wtResume;
    WTexture wtLogOut;
    WTexture wtQuit;
} PauseScreen;

void pause_new(PauseScreen* ps, SDL_Renderer* renderer, TTF_Font* font);
void pause_render(PauseScreen* ps, SDL_Renderer* renderer, int width, int height);

#endif //PAUSE_H