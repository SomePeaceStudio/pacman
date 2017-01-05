#include "wtexture.h"
#include "SDL2/SDL_image.h"

WTexture* wtexture_fromFile(SDL_Renderer* renderer, const char* path) {
    WTexture* wtexture = malloc(sizeof(WTexture));
    SDL_Texture* sdlTexture = NULL;

	//Ielādē attēlu no faila
	SDL_Surface* loadedSurface = IMG_Load(path);
	if( loadedSurface == NULL ) {
		printf("Unable to load image %s! SDL_image Error: %s\n", path, IMG_GetError());
        exit (1);
	}
    
	//Uzstāda attēlam caurspīdīgo krāsu - to, kas nav jārenderē
	SDL_SetColorKey( loadedSurface, SDL_TRUE, 0);

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
    return wtexture;
}

void wtexture_free(WTexture* wtexture) {
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