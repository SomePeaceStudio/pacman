#ifndef STRING_H
#define STRING_H

typedef struct {
    char* buffer;
    size_t bufferLength;
} string;

//Izveido jaunu string ar padoto tekstu (text var būt NULL)
void str_new(string* str, const char* text);

//Atbrīvo bufera resursus
void str_free(string* str);

//Pievieno galā jaunu simbolu
void str_appendChar(string* str, char c);

void str_appendString(string* str, char* other);

//Noņem no beigām vienu simbolu
void str_popBack(string* str);

//Izdrukā string (debugošanai)
void str_print(string* str);

#endif //STRING_H
