#include "texture.h"
#include "SDL2/SDL_image.h"

Texture* texture_fromFile(SDL_Renderer* rederer, const char* path) {
    Texture* texture = malloc(sizeof(Texture));
    SDL_Texture* newTexture = NULL;

	//Ielādē attēlu no faila
	SDL_Surface* loadedSurface = IMG_Load(path);
	if( loadedSurface == NULL ) {
		printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError() );
        exit 1;
	}
    
	//Uzstāda attēlam krāsu
	// SDL_SetColorKey( loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0, 0xFF, 0xFF));

	//Pārveido ielādēto texture priekš renderēšanas
    newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
	if( newTexture == NULL ) {
		printf( "Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError() );
        exit 1;
	}
    
	//Uzstāda garumu, platumu
	texture->width = loadedSurface->w;
	texture->height = loadedSurface->h;

	//Atbrīvojas no ielādētā texture
	SDL_FreeSurface(loadedSurface);

	//Return success
	texture->texture = newTexture;
    return texture;
}

void texture_render(Texture* tx, SDL_Renderer* renderer, int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip)
{
	//Set rendering space and render to screen
	SDL_Rect renderQuad = { x, y, tx->width, tx->height };

	//Set clip rendering dimensions
	if( clip != NULL )
	{
		renderQuad.w = clip->w;
		renderQuad.h = clip->h;
	}

	//Render to screen
	SDL_RenderCopyEx(renderer, tx->texture, clip, &renderQuad, angle, center, flip );
}

void texture_free(Texture* tx) {
    free(tx->texture);
    free (tx);
}