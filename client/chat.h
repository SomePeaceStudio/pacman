#ifndef CHAT_H
#define CHAT_H

#include <stdlib.h>
#include <SDL2/SDL.h>

#include "wtexture.h"

typedef struct ChatMessage {
    char* sender;
    char* message;
    struct ChatMessage* next;
    WTexture texture; //Katrai ziņai ir arī texture, ko rādīt uz ekrāna
} ChatMessage;

//Saistītais saraksts ar čata ziņām (darbojas kā rinda)
typedef struct {
    int capacity;           //Norāda čata ziņu max skaitu
    int count;              //Norāda, cik ziņas pašlaik ir struktūrā
    ChatMessage* first;
    ChatMessage* last;
} Chat;

void chat_new(Chat* chat, size_t capacity);
void chat_add(Chat* chat, const char* sender, const char* message);
void chat_render(Chat* chat, SDL_Renderer* renderer, TTF_Font* font, int windowHeight);
void chat_print(Chat* chat); //Debugošanai

#endif //CHAT_H
