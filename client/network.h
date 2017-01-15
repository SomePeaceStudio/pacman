#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>

#include "../shared/shared.h"
#include "player.h"
#include "tile.h"

//Struktūra, ko padot net_handleTcpPackets un net_handleUdpPackets metodēm
typedef struct {
    sockets_t sockets;
    uint32_t tileCount;
    uint32_t levelWidth;
    Player* me;
    Tile* tiles;
    struct hashmap* hm_players;
} thread_args_t;

void net_sendMove(int socket, int playerId, char direction);
void net_sendChat(int socket, int playerId, const char* message);

//Apstrādā visus saņemtos datus no servera.
//Domātas palaišanai jaunā pavedienā
//Arguments ir norāde uz thread_args_t struktūru
void* net_handleUdpPackets(void* arg);
void* net_handleTcpPackets(void* arg);

#endif //NETWORK_H
