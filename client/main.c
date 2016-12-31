#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "login.h"
#include "game.h"

int main(int argc, char* argv[]) {
    //TCP un UDP soketi
    sockets_t sock;
    sock.tcp = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock.tcp < 0) {
        Die(ERR_SOCKET);
    }
    
    //Servera adrese
    struct sockaddr_in serverAddress;
    //Spēlētāja struktūra
    Player player;
    
    login_window* loginWindow = login_createWindow(
        &argc, &argv, &sock, &serverAddress, &player
    );
    
    login_showWindow(loginWindow);
    login_waitForResult(loginWindow);
    
    if (loginWindow->loggedIn)
        printf("Success\n");
    else
        printf("Me sad\n");
    
    if (!loginWindow->loggedIn) {
        debug_print("%s\n", "User closed login window.");
        return 0;
    }
    
    game_showMainWindow(&player, &sock, &serverAddress);
    
    return 0;
}