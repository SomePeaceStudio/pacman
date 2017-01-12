#ifndef NETWORK_H
#define NETWORK_H

void net_sendMove(int socket, int playerId, char direction);
void net_sendChat(int socket, int playerId, const char* message);

#endif //NETWORK_H
