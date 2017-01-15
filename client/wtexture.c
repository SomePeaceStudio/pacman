#include <SDL2/SDL_image.h>

#include "wtexture.h"
#include "../shared/shared.h"


void wtexture_fromFile(WTexture* wtexture, SDL_Renderer* renderer, const char* path) {
    if (wtexture == NULL) {
        wtexture = malloc(sizeof(WTexture));
        if (wtexture == NULL) {
            Die(ERR_MALLOC);
        }
    } else {
        wtexture_free(wtexture);
    }
    
    SDL_Texture* sdlTexture = NULL;

	//Ielādē attēlu no faila
	SDL_Surface* loadedSurface = IMG_Load(path);
	if(loadedSurface == NULL) {
		printf("Unable to load image %s! SDL_image Error: %s\n", path, IMG_GetError());
        exit (1);
	}
    
	//Uzstāda attēlam caurspīdīgo krāsu - to, kas nav jārenderē
	SDL_SetColorKey(loadedSurface, SDL_TRUE, 0);

	//Pārveido ielādēto texture priekš renderēšanas
    sdlTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
	if(sdlTexture == NULL) {
		printf( "Unable to create texture from %s! SDL Error: %s\n", path, SDL_GetError() );
        exit (1);
	}
    
	//Uzstāda garumu, platumu
	wtexture->width = loadedSurface->w;
	wtexture->height = loadedSurface->h;

	//Atbrīvojas no ielādētā texture
	SDL_FreeSurface(loadedSurface);

	//Return success
	wtexture->texture = sdlTexture;
}

bool wtexture_fromText(WTexture* wtexture, SDL_Renderer* renderer, TTF_Font* font, SDL_Color textColor, SDL_Color bgColor, const char* text) {
    if (wtexture == NULL) {
        wtexture = malloc(sizeof(WTexture));
        if (wtexture == NULL) {
            Die(ERR_MALLOC);
        }
    } else {
        wtexture_free(wtexture);
    }
    
    //Render text surface
	SDL_Surface* textSurface = TTF_RenderText_Shaded(font, text, textColor, bgColor);
	if( textSurface == NULL ) {
		debug_print("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
	}
	else {
        wtexture->texture = SDL_CreateTextureFromSurface(renderer, textSurface);
		if( wtexture->texture == NULL ) {
			printf( "Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError() );
		}
		else {
			//Get image dimensions
			wtexture->width = textSurface->w;
			wtexture->height = textSurface->h;
		}

		//Get rid of old surface
		SDL_FreeSurface(textSurface);
	}
	
	//Return success
	return wtexture->texture != NULL;
}

void wtexture_free(WTexture* wtexture) {
    if (wtexture == NULL)
        return;
    
    if (wtexture->texture != NULL) {
        SDL_DestroyTexture(wtexture->texture);
        wtexture->texture = NULL;
        wtexture->width = wtexture->height = 0;
    }
}

void wtexture_render(
    WTexture* wtexture,
    SDL_Renderer* renderer,
    int x,
    int y,
    SDL_Rect* clip,
    double angle,
    SDL_Point* center,
    SDL_RendererFlip flip
) {
    
    //Set rendering space and render to screen
	SDL_Rect renderQuad = { x, y, wtexture->width, wtexture->height };

	//Set clip rendering dimensions
	if( clip != NULL ) {
		renderQuad.w = clip->w;
		renderQuad.h = clip->h;
	}
    
	//Render to screen
	SDL_RenderCopyEx(
        renderer,
        wtexture->texture,
        clip,
        &renderQuad,
        angle, 
        center,
        flip
    );
}