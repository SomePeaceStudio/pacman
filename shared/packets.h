#ifndef PACKETS_H
#define PACKETS_H

//Pakešu tipi, kas definēti protokolā
typedef enum {
    PT_JOIN = 0,
    PT_ACK = 1,
    PT_START = 2,
    PT_END = 3,
    PT_MAP = 4,
    PT_PLAYERS = 5,
    PT_SCORE = 6
} PacketType;

#endif /* PACKETS_H */