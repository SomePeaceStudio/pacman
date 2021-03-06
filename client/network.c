#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "network.h"
#include "../shared/hashmap.h"

//Nodeklarē vajadzīgās hashmap funkcijas
HASHMAP_FUNCS_DECLARE(int, int32_t, Player);

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
    strncpy(&pack[9], message, messageLength); //9. - pēdējais baits
    safeSend(socket, pack, packetLength, 0);
}

void net_sendQuit(int socket, int32_t playerId) {
    char* pack = allocPack(PSIZE_QUIT);        
    pack[0] = PTYPE_QUIT;                      //0. baits
    itoba(playerId, &pack[1]);                 //1. - 4. baits
    safeSend(socket, pack, PSIZE_QUIT, 0);
}

void* net_handleUdpPackets(void* arg) {
    thread_args_t* convertedArgs = (thread_args_t*)arg;
    Player* playerPtr;
    int32_t playerCount;
    char packType;
    char* currentByte;
    char* pack;
    
    size_t packSize;
    //Ja karte ir pārāk liela, piemēram, 100x100, tad tā neietilpst iekš 
    //  DEFAULT_PACK_SIZE un buferi vajag lielāku.
    if (convertedArgs->tileCount + 1 > DEFAULT_PACK_SIZE) {
        packSize = convertedArgs->tileCount + 1;
    } else {
        packSize = DEFAULT_PACK_SIZE;
    }
        
    pack = malloc(packSize);
    
    while (!*convertedArgs->quit) {
        int received = safeRecv(convertedArgs->sockets.udp, pack, packSize, 0);
        
        //Neizdevās nolasīt datus, sasniegts timeout (vai cits iemesls)
        if (received <= 0) {
            continue;
        }
        
        switch (pack[0]) {
        //Semikols aiz kola ir nepieciešams, jo pēc label ir jāsako statement un
        //  int x = 0, y = 0 neskaitās kā statement. Ievietojam tukšu statement.
        case PTYPE_MAP:;
            //X un Y nobīdes kartē
            int x = 0, y = 0;
            for (int i = 0; i < convertedArgs->tileCount; i++) {
                tile_new(&convertedArgs->tiles[i], x, y, pack[i+1]);
                
                x += TILE_SPRITE_SIZE;
                
                //"Pārlec" uz jaunu rindu
                if (x >= *convertedArgs->levelWidth) {
                    x = 0;
                    y += TILE_SPRITE_SIZE;
                }
            }
            break;
        
            
        //Saņemta PLAYERS pakete
        case PTYPE_PLAYERS:
            //Paketes otrais līdz piektais baits nosaka spēlētāju skaitu
            playerCount = batoi(&pack[1]);
            
            //Kursors, kurš nosaka vietu, līdz kurai pakete nolasīta
            currentByte = &pack[5];
            
            for (int i = 0; i < playerCount; i++) {
                bool newPlayer = false;
                
                int id = batoi(currentByte);
                currentByte += 4;
                
                if (id == convertedArgs->me->id) {
                    playerPtr = convertedArgs->me;
                } else {
                    playerPtr = hashmap_int_get(convertedArgs->hm_players, &id);
                    
                    //Spēlētājs netika atrasts, tātad jāveido jauns
                    if (playerPtr == NULL) {
                        playerPtr = calloc(1, sizeof(Player));
                        if (playerPtr == NULL) {
                            Die(ERR_MALLOC);
                        }
                        newPlayer = true;
                    }
                }
                
                //Atjaunojam spēlētāja informāciju
                playerPtr->x = batof(currentByte) * PLAYER_SPRITE_SIZE;
                currentByte += 4;
                
                playerPtr->y = batof(currentByte) * PLAYER_SPRITE_SIZE;
                currentByte += 4;
                
                playerPtr->state = *currentByte;
                currentByte++;
                
                playerPtr->type = *currentByte;
                currentByte++;
                
                if (newPlayer) {
                    hashmap_int_put(convertedArgs->hm_players, &id, playerPtr);
                } else if (playerPtr == convertedArgs->me) {
                    player_setCamera(
                        convertedArgs->me,
                        convertedArgs->camera,
                        *convertedArgs->windowWidth,
                        *convertedArgs->windowHeight,
                        *convertedArgs->levelWidth,
                        *convertedArgs->levelHeight
                    );
                }
            }
            break;
        
            
        //Saņemta SCORE pakete
        case PTYPE_SCORE:
            //nulltais baits ir tipam, tāpēc sākam lasīt no pirmā
            currentByte = &pack[1];
            
            playerCount = batoi(currentByte);
            currentByte += 4;
            
            for (int i = 0; i < playerCount; i++) {
                int32_t score = batoi(currentByte);
                currentByte += 4;
                
                int32_t id = batoi(currentByte);
                currentByte += 4;
                
                if (id ==convertedArgs->me->id) {
                    convertedArgs->me->score = score;
                } else {
                    playerPtr = hashmap_int_get(convertedArgs->hm_players, &id);
                    if (playerPtr != NULL) {
                        playerPtr->score = score;
                    }
                }
            }
            
            break;
        }
    }
    
    debug_print("%s\n", "UDP thread cleanup");
    free(pack);
}

void* net_handleTcpPackets(void* arg) {
    thread_args_t* convertedArgs = (thread_args_t*)arg;
    
    int32_t playerId;
    char packType;
    char* pack;
    size_t remainingLength;
    
    while (!*convertedArgs->quit) {
        ssize_t received = recv(convertedArgs->sockets.tcp, &packType, 1, 0);
        
        //Neizdevās nolasīt paketes tipu, sasniegts timeout (vai cits iemesls)
        if (received != 1) {
            continue;
        }

        switch (packType) {
        
        //Pēc label ievietots tukšs statement, lai nebūt compile-time kļūda
        case PTYPE_JOINED:;
            remainingLength = PSIZE_JOINED - 1; //1 baits ir paketes tipam
            char* cursor;
            pack = allocPack(remainingLength);
            
            safeRecv(convertedArgs->sockets.tcp, pack, remainingLength , 0);
            cursor = &pack[0];
            
            int32_t id = batoi(cursor);
            cursor += 4;
            
            char nick[21];
            snprintf(nick, 21, "%s", cursor);
            
            Player* pl = hashmap_int_get(convertedArgs->hm_players, &id);
            
            if (pl == NULL) {
                pl = malloc(sizeof(Player));
                if (pl == NULL) {
                    Die (ERR_MALLOC);
                }
                pl->id = id;
                hashmap_int_put(convertedArgs->hm_players, &id, pl);
            }
            
            size_t nickLength = strlen(nick);
            pl->nick = malloc(nickLength + 1);
            strncpy(pl->nick, nick, nickLength);
            pl->nick[nickLength + 1] = '\0';
            
            free(pack);
            break;
            
            
        case PTYPE_PLAYER_DISCONNECTED:;
            remainingLength = PSIZE_PLAYER_DISCONNECTED - 1;
            pack = allocPack(remainingLength);
            safeRecv(convertedArgs->sockets.tcp, pack, remainingLength, 0);
            playerId = batoi(pack);
            hashmap_int_remove(convertedArgs->hm_players, &playerId);
            free (pack);
            break;
            
        case PTYPE_MESSAGE:
            printf("OMG, chat message!\n");
            //Nolasa spēlētaja id un ziņas garumu
            pack = allocPack(8);
            safeRecv(convertedArgs->sockets.tcp, pack, 8, 0);
            playerId = batoi(pack);
            uint32_t messageLength = batoi(&pack[4]);
            free(pack);
            
            //Nolasa atlikušo paketi (spēlētāja atstāto ziņu)
            pack = allocPack(messageLength + 1);
            safeRecv(convertedArgs->sockets.tcp, pack, messageLength, 0);
            pack[messageLength] = '\0';
            
            //Atrod sūtītāja niku
            const char* senderNick = NULL;
            if (playerId == convertedArgs->me->id) {
                senderNick = convertedArgs->me->nick;
            } else {
                printf("Chat from other player\n");
                Player* playerPtr = hashmap_int_get(convertedArgs->hm_players, &playerId);
                if (playerPtr != NULL) {
                    printf("Found the one\n");
                    senderNick = playerPtr->nick;
                }
            }
            
            if (senderNick == NULL) {
                senderNick = "UNKNOWN";
            }
            
            //Pievieno ziņu čatam
            if (senderNick != NULL) {
                debug_print("Message from %s: %s\n", senderNick, pack);
                chat_add(convertedArgs->chat, senderNick, pack);
            }
            
            free(pack);
            break;
        }
    }
    
    debug_print("%s\n", "TCP thread cleanup");
}