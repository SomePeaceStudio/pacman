# pacman
The Pacman multiplayer game where players can play as a Pacman or a ghost, the rules are similar to [original Pacman][orig_pac] game.
# Client & Server
Client:
[![Pacman Client](https://raw.githubusercontent.com/SomePeaceStudio/packman/new-prot/pacman-client.png)](https://raw.githubusercontent.com/SomePeaceStudio/packman/new-prot/pacman-client.png)

Server:
[![Pacman Server](https://raw.githubusercontent.com/SomePeaceStudio/packman/new-prot/pacman-server.png)](https://raw.githubusercontent.com/SomePeaceStudio/packman/new-prot/pacman-server.png)

# To Build
Before you compile check that you have installed necessary libraries that the client uses:
```sh
$ sudo apt-get install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libgtk-3-dev
```

Code is compiled using the gcc compiler by default. To compile the client and server, simply run `make` in the project root directory. 

The `makefile` also contains the following rules:
* client + server (default)
* client
* server
* clean
* rebuild

# To Run

Server:
```sh
/pacman$ bin/server < port > < map, example: maps/21x21 >
```

Client:
Note: the client must be run from the `bin` folder.
```sh
/pacman/bin$ client_gui
```

There is also a 'console' version of the client (w,a,s,d + Enter to move):
```sh
/pacman/bin$ client < server address, example: 127.0.0.1 > < port >
```

[orig_pac]: https://en.wikipedia.org/wiki/Pac-Man
