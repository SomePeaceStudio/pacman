#Kompilators
CC=gcc

#Opcijas, ko padot kompilatoram.
#-lm: lai varētu izmantot math.h	
CFLAGS=-pthread -lm

#Bibliotēkas, ko izmanto gui klients
LIBS_CLIENT_GUI=`pkg-config --libs gtk+-3.0` -lSDL2 -lSDL2_image -lSDL2_ttf
#Kompilatora opcijas priekš gui klienta
CFLAGS_CLIENT_GUI=`pkg-config --cflags gtk+-3.0`

#Gui klientam nepieciešamie faili
HEADERS_CLIENT_GUI=client/login.h client/game.h client/tile.h client/player.h client/network.h shared/hashmap.h
SOURCES_CLIENT_GUI=client/main.c client/login.c client/game.c client/tile.c client/wtexture.c shared/shared.c client/player.c client/network.c shared/hashmap.c

#Izpildāmo failu nosaukumi
BIN_CLIENT=bin/client
BIN_CLIENT_GUI=bin/client_gui
BIN_SERVER=bin/server

all: mkdir server client client_gui

client: client/client.c shared/shared.h mkdir
	$(CC) client/client.c shared/shared.c $(CFLAGS) -o $(BIN_CLIENT)  

client_gui: client/main.c $(HEADERS_CLIENT_GUI) mkdir
	$(CC) $(CFLAGS_CLIENT_GUI) $(SOURCES_CLIENT_GUI) -o $(BIN_CLIENT_GUI) $(LIBS_CLIENT_GUI)
	cp client/res/tiles.png bin/
	cp client/res/players.png bin/
	cp client/res/font.ttf bin/

server: server/server.c shared/shared.h shared/threads.h mkdir
	$(CC) server/server.c shared/shared.c shared/threads.c $(CFLAGS) -o $(BIN_SERVER) 

clean:
	rm -r bin

rebuild: clean all

mkdir:
	mkdir -p bin

debug: CFLAGS+= -g
debug: CFLAGS_CLIENT_GUI+= -g
debug: all