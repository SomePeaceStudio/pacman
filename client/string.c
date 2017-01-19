#include <stdio.h>
#include <string.h>

#include "string.h"
#include "../shared/shared.h"

//Nosaka, par cik simboliem katru reizi tiek palielināts string buferis
#define REALLOC_INCREMENT 10

void str_new(string* str, const char* text) {
    if (str == NULL) {
        str = malloc(sizeof(string));
        if (str == NULL) {
            Die(ERR_MALLOC);
        }
    }
    
    //Inicializē tukšu string
    if (text == NULL) {
        str->buffer = NULL;
        str->bufferLength = 0;
        return;
    }
    
    str->bufferLength = strlen(text) + 1;
    str->buffer = malloc(str->bufferLength);
    if (str->buffer == NULL) {
        Die(ERR_MALLOC);
    }
    strcpy(str->buffer, text);
}


void str_free(string* str) {
    free (str->buffer);
}


void str_append(string* str, char c) {
    if (str == NULL) {
        return;
    }
    
    //Ja ir tikko izveidots tukšs string
    if (str->buffer == NULL) {
        str->buffer = malloc(str->bufferLength = REALLOC_INCREMENT);
        if (str->buffer == NULL) {
            Die(ERR_MALLOC);
        }
        str->buffer[0] = c;
        str->buffer[1] = '\0';
        return;
    }
    
    //Palielina buferi, ja nepieciešams
    size_t stringLength = strlen(str->buffer);
    if (stringLength + 1 == str->bufferLength) { //+1 priekš '\0' simbola
        str->bufferLength += REALLOC_INCREMENT;
        str->buffer = realloc(str->buffer, str->bufferLength);
        if (str->buffer == NULL) {
            Die(ERR_MALLOC);
        }
    }
    
    //Pabīda string beigas par 1 vietu uz priekšu
    str->buffer[stringLength] = c;
    str->buffer[stringLength + 1] = '\0';
}


void str_popBack(string* str) {
    if (str == NULL || str->buffer == NULL) {
        return;
    }
    
    size_t stringLength = strlen(str->buffer);
    if (stringLength > 0) { //Vismaz 1 simbols
        str->buffer[stringLength - 1] = '\0';
    }
}


void str_print(string* str) {
    if (str == NULL || str->buffer == NULL) {
        return;
    }
    
    printf("%s\n", str->buffer);
}