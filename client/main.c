#include <stdio.h>
#include <stdbool.h>

#include "login.h"
#include "mainWindow.h"
#include "globals.h"

void clean() {
    free (gPlayerName);
}

int main(int argc, char* argv[]) {
    
    //Pagaidām parāda tikai pieslēgšanās logu
    bool loggedIn = showLoginForm(&argc, &argv);
    
    if (loggedIn) {
        show_main_window();
    }
    
    clean();
    return 0;
}