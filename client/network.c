#include <stdlib.h>
#include <string.h>

#include "network.h"
#include "../shared/shared.h"

void net_sendMove(int socket, int playerId, char direction) {
    char* pack = allocPack(PSIZE_MOVE);
    pack[0] = PTYPE_MOVE;                   //0. baits
    itoba(playerId, &pack[1]);              //1. - 4. baits
    pack[5] = direction;                    // 5.baits
    safeSend(socket, pack, PSIZE_MOVE, 0);
}

void net_sendChat(int socket, int playerId, const char* message) {
    size_t messageLength = strlen(message);
    //9 baiti = tips, spēlētāja id, ziņas garums
    size_t packetLength = 9 + messageLength;
    unsigned char* pack = allocPack(packetLength);
    
    pack[0] = PTYPE_MESSAGE;                   //0. baits
    itoba(playerId, &pack[1]);                 //1. - 4. baits
    itoba(messageLength, &pack[5]);            //5. - 8. baits
    strncpy(&pack[6], message, messageLength); //6. - pēdējais baits
    safeSend(socket, pack, packetLength, 0);
}