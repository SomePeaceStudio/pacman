#ifndef GLOBALS_H
#define  GLOBALS_H

// ========================================================================= //
//                  Klienta puses globālie mainīgie
// ========================================================================= //

#include <stdlib.h>

//Soketi
extern int gSockUdp;
extern int gSockTcp;

//Spēles mainīgie
extern int32_t gPlayerId;
extern char* gPlayerName;
extern int gMapWidth;
extern int gMapHeight;
extern int gPlayerX;
extern int gPlayerY;
extern char** gMap;

#endif //GLOBALS_H