#include "pausescreen.h"

void pause_new(PauseScreen* ps, SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Color textColor = {.r = 255, .g = 255, .b = 255};
    SDL_Color bgColor = {.r = 0, .g = 0, .b = 0};
    
    wtexture_fromText(&ps->wtResume, renderer, font, textColor, bgColor, "1: Resume");
    wtexture_fromText(&ps->wtLogOut, renderer, font, textColor, bgColor, "2: Log out");
    wtexture_fromText(&ps->wtQuit, renderer, font, textColor, bgColor, "3: Quit");
}

void pause_render(PauseScreen* ps, SDL_Renderer* renderer, int width, int height) {
    SDL_Rect background = {.w = width, .h = height, .x = 0, .y = 0};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_RenderFillRect(renderer, &background);
    
    wtexture_render(&ps->wtResume, renderer, 10, 10, NULL);
    wtexture_render(&ps->wtLogOut, renderer, 10, 10 + ps->wtResume.height, NULL);
    wtexture_render(&ps->wtQuit, renderer, 10, 10 + ps->wtResume.height * 2, NULL);
}