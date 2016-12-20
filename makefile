#Kompilators
CC=gcc

#Opcijas, ko padot kompilatoram.
#-fshort-enums: lai enum vērtības aizņemtu pēc iespējas mazāk baitu.
#-lm: lai varētu izmantot math.h	
CFLAGS=-pthread -fshort-enums -lm

#Izpildāmo failu nosaukumi
BIN_CLIENT=bin/client
BIN_SERVER=bin/server

all: mkdir client server 

client: client/client.c shared/shared.h mkdir
	$(CC) client/client.c shared/shared.c $(CFLAGS) -o $(BIN_CLIENT)  

server: server/server.c shared/shared.h mkdir
	$(CC) server/server.c shared/shared.c $(CFLAGS) -o $(BIN_SERVER) 

clean:
	rm -r bin

rebuild: clean all

mkdir:
	mkdir -p bin