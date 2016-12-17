#Kompilators
CC=gcc

#Opcijas, ko padot kompilatoram.
#-fshort-enums: lai enum vērtības aizņemtu pēc iespējas mazāk baitu.
CFLAGS=-pthread -fshort-enums

#Izpildāmo failu nosaukumi
BIN_CLIENT=bin/client
BIN_SERVER=bin/server

all: mkdir client server 

client: client/client.c shared/shared.h mkdir
	$(CC) $(CFLAGS) -o $(BIN_CLIENT)  client/client.c shared/shared.c

server: server/server.c shared/shared.h mkdir
	$(CC) $(CFLAGS) -o $(BIN_SERVER) server/server.c shared/shared.c

clean:
	rm -r bin

rebuild: clean all

mkdir:
	mkdir -p bin