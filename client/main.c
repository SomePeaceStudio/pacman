#include <stdio.h>

#include "login.h"
#include "globals.h"

void clean() {
    free (gPlayerName);
}

int main(int argc, char* argv[]) {
    
    //Pagaidām parāda tikai pieslēgšanās logu
    int loginResult = showLoginForm(argc, argv);
    
    printf("Player name: %s\n", gPlayerName);
    printf("Player id: %d\n", gPlayerId);
    
    clean();
    
    return loginResult;
}