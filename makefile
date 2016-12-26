#Kompilators
CC=gcc

#Opcijas, ko padot kompilatoram.
#-lm: lai varētu izmantot math.h	
CFLAGS=-pthread -lm

#Izpildāmo failu nosaukumi
BIN_CLIENT=bin/client
BIN_CLIENT_GUI=bin/client_gui
BIN_SERVER=bin/server

all: mkdir client server 

client: client/client.c shared/shared.h mkdir
	$(CC) client/client.c shared/shared.c $(CFLAGS) -o $(BIN_CLIENT)  

client_gui: client/gui_login.c shared/shared.h mkdir
	$(CC) `pkg-config --cflags gtk+-3.0` client/gui_login.c shared/shared.c -o $(BIN_CLIENT_GUI) `pkg-config --libs gtk+-3.0`

server: server/server.c shared/shared.h shared/threads.h mkdir
	$(CC) server/server.c shared/shared.c shared/threads.c $(CFLAGS) -o $(BIN_SERVER) 

clean:
	rm -r bin

rebuild: clean all

mkdir:
	mkdir -p bin