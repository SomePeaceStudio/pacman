#include <SDL2/SDL.h>
#include <stdbool.h>

#include "mainWindow.h"
#include "../shared/shared.h"

void show_main_window() {
    // while(1){}
    SDL_Window* window = NULL;
    SDL_Surface* surface = NULL;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        Die("Failed to initialize SDL\n");
    }
    
    window = SDL_CreateWindow(
        "Pacman",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    
    if (window == NULL) {
        Die("Failed to creade SDL window\n");
    }
    
    bool quit = false;
    SDL_Event e;
    
    surface = SDL_GetWindowSurface(window);
    SDL_FillRect( surface, NULL, SDL_MapRGB( surface->format, 0xFF, 0xFF, 0xFF ) );
    SDL_UpdateWindowSurface( window );
    
    //Main loop
    while (!quit) {
        
        //Apstrādā notikumus events()
        while( SDL_PollEvent( &e ) != 0 ) {
            if(e.type == SDL_QUIT) {
                quit = true;
            }
        }
    }
    
    SDL_DestroyWindow( window );
}