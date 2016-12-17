# pacman

##Building
Code is compiled using gcc compiler by default. To compile client and server, simply run `make` in the project root directory.

The makefile also contains the following rules:
* client + server (default)
* client
* server
* clean
* rebuild

##Running
Client:
```
bin/client 127.0.0.1 < port >
```

Server:
```
bin/server < port > map
```
