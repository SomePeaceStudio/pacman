#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>

#include "../shared/shared.h"
#include "player.h"
#include "tile.h"
#include "chat.h"

//Struktūra, ko padot net_handleTcpPackets un net_handleUdpPackets metodēm
typedef struct {
    sockets_t sockets;           //TCP un UDP soketi
    uint32_t tileCount;          //tiles masīva izmērs
    uint32_t* levelWidth;        //Līmeņa (kartes) platums
    uint32_t* levelHeight;       //Līmeņa (kartes) augstums
    uint32_t* windowWidth;       //Loga platums
    uint32_t* windowHeight;      //Loga augstums
    SDL_Rect* camera;            //Kameras taisnstūris (domāts apcirpšanai)
    Player* me;                  //Pats spēlētājs
    bool* quit;                  //Nosaka, vai lietotājs vēlas iziet no spēles
    Chat* chat;
    Tile* tiles;                 //Spēles karte
    struct hashmap* hm_players;  //Pārējie spēlētāji
} thread_args_t;

void net_sendMove(int socket, int playerId, char direction);
void net_sendChat(int socket, int playerId, const char* message);
void net_sendQuit(int socket, int32_t playerId);

//Apstrādā visus saņemtos datus no servera.
//Domātas palaišanai jaunā pavedienā
//Arguments ir norāde uz thread_args_t struktūru
void* net_handleUdpPackets(void* arg);
void* net_handleTcpPackets(void* arg);

#endif //NETWORK_H
