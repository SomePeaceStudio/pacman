#include <string.h>
#include <stdio.h>

#include "chat.h"
#include "../shared/shared.h"

SDL_Color txtColor = { .r = 255, .g = 255, .b = 255, .a = 200 };
SDL_Color bgColor  = { .r = 0, .g = 0, .b = 0, .a = 150 };

void chat_new(Chat* chat, size_t capacity) {
    if (chat == NULL) return;
    
    if (capacity < 1) {
        capacity = 1;
    }
    
    chat->capacity = capacity;
    chat->count = 0;
    chat->first = chat->last = NULL;
}

void chat_add(Chat* chat, const char* sender, const char* message) {
    if (chat == NULL) return;
    
    ChatMessage* chatMessage = safeMalloc(sizeof(ChatMessage));
    chatMessage->next = NULL;
        
    //Uzstāda sūtītāju
    chatMessage->sender = safeMalloc(strlen(sender) + 1);
    strcpy(chatMessage->sender, sender);
    
    //Uzstāda ziņu
    chatMessage->message = safeMalloc(strlen(message) + 1);
    strcpy(chatMessage->message, message);
    
    wtexture_free(&chatMessage->texture);
    
    //Ja čats ir tukšs
    if (chat->first == NULL) {
        chat->first = chat->last = chatMessage;
    } else {
        chat->last->next = chatMessage;
        chat->last = chatMessage;
    } 
    
    chat->count++;
    
    //Nav jāuztraucas par robežgadījumiem, jo šis izpildīsies tikai tad, ja
    //  sarakstā būs vismaz 2 elementi
    if (chat->count > chat->capacity) {
        ChatMessage* p = chat->first;
        chat->first = p->next;
        
        free(p->message);
        free(p->sender);
        free(p);
    }
}

void chat_render(Chat* chat, SDL_Renderer* renderer, TTF_Font* font, int windowHeight) {
    ChatMessage* current;
    int yPos = 8;
    for(current = chat->first; current != NULL; current = current->next) {
        if (current->texture.texture == NULL) {
            size_t lineLength = strlen(current->sender) + strlen(current->message) + 3;
            char* chatLine = safeMalloc(lineLength);
            snprintf(chatLine, lineLength, "%s: %s", current->sender, current->message);
            wtexture_fromText(&current->texture, renderer, font, txtColor, bgColor, chatLine);
            free(chatLine);
        }
        
        wtexture_render(&current->texture, renderer, 8, yPos, NULL);
        yPos += 15;
    }
}

void chat_print(Chat* chat) {
    ChatMessage* current;
    for (current = chat->first; current != NULL; current = current->next) {
        printf("%s: %s\n", current->sender, current->message);
    }
}