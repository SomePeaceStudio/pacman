#ifndef PLAYER_H
#define PLAYER_H

#include "wtexture.h"

//Spēlētāja ikonas jeb texture paltums un augstums
#define PLAYER_SPRITE_SIZE 50
#define TOTAL_PLAYER_SPRITES 2

typedef struct {
    int32_t id;
    float x;
    float y;
    int32_t score;
    unsigned char state;
    unsigned char type;
    char* nick;
} Player;

//Šis jāizsauc pirms var renderēt spēlētājus
void player_init();
void player_render(Player* player, SDL_Rect* camera, WTexture* wt, SDL_Renderer* renderer);

#endif //PLAYER_H