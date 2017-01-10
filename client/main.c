#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "login.h"
#include "game.h"

int main(int argc, char* argv[]) {
    //TCP un UDP soketi
    sockets_t sock;
    struct sockaddr_in serverAddress, clientAddr;
    socklen_t clientAddrLength;
    //Spēlētāja struktūra
    Player player;
    
    //izveido TCP soketu
    sock.tcp = socket(PF_INET, SOCK_STREAM, 0);
    if (sock.tcp < 0) {
        Die(ERR_SOCKET);
    }
    
    //Izveido UDP soketu
    sock.udp = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock.udp < 0) {
        Die(ERR_SOCKET);
    }
    
    //Izveido pieslēgšanās logu un sagaida rezultātu no tā
    login_window* loginWindow = login_createWindow(
        &argc, &argv, &sock, &serverAddress, &player
    );
    login_showWindow(loginWindow);
    login_waitForResult(loginWindow);
    
    //Ja lietotājs aizvēra logu
    if (!loginWindow->loggedIn) {
        debug_print("%s\n", "User closed login window.");
        return 0;
    }
    
    //Bindojam UDP soketu pie TCP adreses
    clientAddrLength = sizeof(clientAddr);
    getsockname(sock.tcp, (struct sockaddr*)&clientAddr, &clientAddrLength);
    if (bind(sock.udp,(struct sockaddr *)&clientAddr, clientAddrLength) < 0) {
        Die(ERR_BIND);
    }
    
    //Savienojas ar serveri caur UDP
    if (connect(sock.udp, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        Die(ERR_CONNECT);
    }
    
    //Parāda galveno spēles logu
    game_showMainWindow(&player, &sock, &serverAddress);

    return 0;
}