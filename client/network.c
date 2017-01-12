#include <stdlib.h>

#include "network.h"
#include "../shared/shared.h"

void net_sendMove(int socket, int playerId, char direction) {
    char* pack = allocPack(PSIZE_MOVE);
    pack[0] = PTYPE_MOVE;
    itoba(playerId, &pack[1]);
    pack[5] = direction;
    safeSend(socket, pack, PSIZE_MOVE, 0);
}