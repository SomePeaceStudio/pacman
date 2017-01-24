#include "player.h"
#include "tile.h"
#include "../shared/shared.h"

//Privātie mainīgie
static SDL_Rect pacmanClips[TOTAL_PLAYER_SPRITES];
static SDL_Rect ghostClips[4];

void player_render(Player* player, SDL_Rect* camera, WTexture* wt, SDL_Renderer* renderer) {
    
    //Izvēlās nepieciešamo apcipršanas apgabalu
    SDL_Rect clip;
    if (player->type == PLTYPE_PACMAN) {
        clip = pacmanClips[player->state];
    } else {
        clip = ghostClips[player->id % 4];
    }
    
    wtexture_render_2(
        wt,
        renderer,
        player->x - camera->x,
        player->y - camera->y,
        &clip,
        0,
        NULL,
        SDL_FLIP_NONE
    );
}

//Inicializē sprite robežas failā res/players.png
void player_init() {
    //Normal pacman
    pacmanClips[PLSTATE_LIVE].x = 0;
    pacmanClips[PLSTATE_LIVE].y = 0;
    pacmanClips[PLSTATE_LIVE].w = PLAYER_SPRITE_SIZE;
    pacmanClips[PLSTATE_LIVE].h = PLAYER_SPRITE_SIZE;
    
    //Dead pacman
    pacmanClips[PLSTATE_DEAD].x = PLAYER_SPRITE_SIZE;
    pacmanClips[PLSTATE_DEAD].y = 0;
    pacmanClips[PLSTATE_DEAD].w = PLAYER_SPRITE_SIZE;
    pacmanClips[PLSTATE_DEAD].h = PLAYER_SPRITE_SIZE;
    
    //Powerup pacman
    pacmanClips[PLSTATE_POWERUP].x = 0;
    pacmanClips[PLSTATE_POWERUP].y = PLAYER_SPRITE_SIZE;
    pacmanClips[PLSTATE_POWERUP].w = PLAYER_SPRITE_SIZE;
    pacmanClips[PLSTATE_POWERUP].h = PLAYER_SPRITE_SIZE;
    
    //Invincible pacman
    pacmanClips[PLSTATE_INVINCIBILITY].x = PLAYER_SPRITE_SIZE;
    pacmanClips[PLSTATE_INVINCIBILITY].y = PLAYER_SPRITE_SIZE;
    pacmanClips[PLSTATE_INVINCIBILITY].w = PLAYER_SPRITE_SIZE;
    pacmanClips[PLSTATE_INVINCIBILITY].h = PLAYER_SPRITE_SIZE;
    
    //Dzeltenais spoks
    ghostClips[0].x = 0;
    ghostClips[0].y = 0;
    ghostClips[0].w = PLAYER_SPRITE_SIZE;
    ghostClips[0].h = PLAYER_SPRITE_SIZE;
    
    //Zilais spoks
    ghostClips[1].x = PLAYER_SPRITE_SIZE;
    ghostClips[1].y = 0;
    ghostClips[1].w = PLAYER_SPRITE_SIZE;
    ghostClips[1].h = PLAYER_SPRITE_SIZE;
    
    //Zaļais spoks
    ghostClips[2].x = 0;
    ghostClips[2].y = PLAYER_SPRITE_SIZE;
    ghostClips[2].w = PLAYER_SPRITE_SIZE;
    ghostClips[2].h = PLAYER_SPRITE_SIZE;
    
    //Sarkanais spoks
    ghostClips[3].x = PLAYER_SPRITE_SIZE;
    ghostClips[3].y = PLAYER_SPRITE_SIZE;
    ghostClips[3].w = PLAYER_SPRITE_SIZE;
    ghostClips[3].h = PLAYER_SPRITE_SIZE;
}

void player_setCamera(
    Player* player,
    SDL_Rect* camera,
    int screenWidth,
    int screenheight,
    int levelWidth,
    int levelHeight) {
    
    camera->x = (player->x + PLAYER_SPRITE_SIZE/2) - screenWidth/2;
    camera->y = (player->y + PLAYER_SPRITE_SIZE/2) - screenheight/2;
    
    camera->x = clipBoth(camera->x, 0, levelWidth - camera->w);
    camera->y = clipBoth(camera->y, 0, levelHeight - camera->h);
}