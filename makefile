#Kompilators
CC=gcc

#Opcijas, ko padot kompilatoram.
#-lm: lai varētu izmantot math.h	
CFLAGS=-pthread -lm

#Bibliotēkas, ko izmanto klients
LIBS_CLIENT=`pkg-config --libs gtk+-3.0` -lSDL2 -lSDL2_image
#Kompilatora opcijas priekš klienta
CFLAGS_CLIENT=`pkg-config --cflags gtk+-3.0`

#Izpildāmo failu nosaukumi
BIN_CLIENT=bin/client
BIN_CLIENT_GUI=bin/client_gui
BIN_SERVER=bin/server

all: mkdir server client client_gui

client: client/client.c shared/shared.h mkdir
	$(CC) client/client.c shared/shared.c $(CFLAGS) -o $(BIN_CLIENT)  

client_gui: client/main.c client/login.h client/game.h client/tile.h mkdir
	$(CC) $(CFLAGS_CLIENT) client/main.c client/login.c client/game.c client/tile.c shared/shared.c -o $(BIN_CLIENT_GUI) $(LIBS_CLIENT)

server: server/server.c shared/shared.h shared/threads.h mkdir
	$(CC) server/server.c shared/shared.c shared/threads.c $(CFLAGS) -o $(BIN_SERVER) 

clean:
	rm -r bin

rebuild: clean all

mkdir:
	mkdir -p bin

debug: CFLAGS+= -g
debug: all