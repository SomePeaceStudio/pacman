#Kompilators
CC=gcc

#Opcijas, ko padot kompilatoram
CFLAGS=-pthread

#Izpildāmo failu nosaukumi
BIN_CLIENT=bin/client
BIN_SERVER=bin/server

all: mkdir client server 

client: client/client.c mkdir
	$(CC) $(CFLAGS) -o $(BIN_CLIENT)  client/client.c

server: server/server.c mkdir
	$(CC) $(CFLAGS) -o $(BIN_SERVER) server/server.c

clean:
	rm -r bin

rebuild: clean all

mkdir:
	mkdir -p bin
