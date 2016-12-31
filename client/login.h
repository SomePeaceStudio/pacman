#ifndef LOGIN_H
#define LOGIN_H

#include <stdbool.h>
#include <SDL2/SDL.h>
#include <gtk/gtk.h>
#include <netinet/in.h>

#include "player.h"
#include "../shared/shared.h"

//Pieslēgšanās loga struktūra
typedef struct {
    GtkWidget* window;
    GtkWidget* grid;
    GtkWidget* addressInput;
    GtkWidget* portInput;
    GtkWidget* nickInput;
    GtkWidget* addressLabel;
    GtkWidget* portLabel;
    GtkWidget* nickLabel;
    GtkWidget* button;
    sockets_t* sock;
    struct sockaddr_in* serverAddress;
    Player* player;
    bool loggedIn;
} login_window;

//Izveido pieslēgšanās logu
login_window* login_createWindow(int* argc,
                                 char** argv[],
                                 sockets_t* sock,
                                 struct sockaddr_in* serverAddress,
                                 Player* player);

//Parāda logu
void login_showWindow(login_window* window);

//Gaida, kamēr lietotājs pieslēdzas serverim, vai aizver logu.
void login_waitForResult(login_window* window);

//Atbrīvo atmiņu
void login_free(login_window* window);

#endif //LOGIN_H