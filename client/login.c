#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <errno.h>

#include "login.h"

//Teksti, ko rādīt lietotājam
#define ERR_ADDRESS "Please enter a valid IP address"
#define ERR_PORT "Please enter a valid port number"
#define ERR_NICK_EMPTY "Please enter a nick"
#define ERR_NICK_TAKEN "Chosen nick is already taken"
#define ERR_NICK_TOO_LONG "Nick is too long (max: 20)"
#define ERR_CONNECTION "Failed to connect to server"
#define ERR_SERVER_FULL "Server is full"

//Parāda kļūdas paziņojumu lietotājam.
//widget: (var būt NULL) lai noteiktu vecāka logu (GtkWindow)
void showError(const char* message, GtkWidget* window) {
    //Izveido un parāda dialogu
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(window), //Dialoga vecāks (logs)
        GTK_DIALOG_MODAL,   //Dialoga tips
        GTK_MESSAGE_ERROR,  //Ziņas tips
        GTK_BUTTONS_CLOSE,  //Poga, kādu parādīt
        "%s", message       //Ziņa, ko parādīt dialogā
    );
    
    //Parāda dialogu
    gtk_dialog_run (GTK_DIALOG (dialog));
    //Šo vajag, lai dialogs pazustu pēc "Close" nospiešanas
    gtk_widget_destroy (dialog);
}

//Argriež spēlētāja id vai kļūdas paziņojumu:
//-1: niks aizņemts
//-2: serveris pilns
//-3: nepareiza / negaidīta atbilde no servera
//-4: ja neizdevās savienoties ar serveri
int tryConnect(login_window* loginWindow) {
    //Mēģina savienoties ar serveri
    if (connect(loginWindow->sock->tcp,
                (struct sockaddr *)loginWindow->serverAddress,
                sizeof(*loginWindow->serverAddress)) < 0) {
        return -4;
    }
    
    unsigned char* pack;
    const char* nick = gtk_entry_get_text(GTK_ENTRY(loginWindow->nickInput));
    
    //1. baits paketes tipam
    pack = allocPack(PSIZE_JOIN);
    pack[0] = PTYPE_JOIN;
    debug_print("Size of name: %ld\n", sizeof(nick));
    memcpy(&pack[1], nick, strlen(nick));
    
    // Send JOIN packet
    debug_print("%s\n", "Sending JOIN packet...");
    safeSend(loginWindow->sock->tcp, pack, PSIZE_JOIN, 0);
    free(pack);
    
    // Receive ACK
    pack = allocPack(PSIZE_ACK);
    
    safeRecv(loginWindow->sock->tcp, pack, PSIZE_ACK, 0);
    debug_print("%s\n", "ACK reveived.");
    
    int32_t id;
    if(pack[0] == PTYPE_ACK) {
        id = batoi(&pack[1]);
    } else {
        //Ja atsūtīta nepareiza tipa pakete
        id = -3;
    }
    
    free(pack);
    return id;
}

//Callback funkcija, kad tiek nospiesta poga "Connect"
static void connectClicked (GtkWidget* widget, gpointer data) {
    login_window* win = (login_window*)data;
    
    // //Dabū ievadītās vērtības no formas
    const char* addressText = gtk_entry_get_text(GTK_ENTRY(win->addressInput));
    const char* portText = gtk_entry_get_text(GTK_ENTRY(win->portInput));
    const char* nickText = gtk_entry_get_text(GTK_ENTRY(win->nickInput));
    
    in_addr_t address = inet_addr(addressText);
    uint16_t port = htons(atoi(portText));
    
    if (address == INADDR_NONE) {
        //Nav svarīgi, kādu GtkWidget padod showError funkcijai
        showError(ERR_ADDRESS, win->window);
        return;
    }
    
    if (strlen(portText) == 0 || port == 0) {
        showError(ERR_PORT, win->window);
        return;
    }
    
    if (strlen(nickText) == 0) {
        showError(ERR_NICK_EMPTY, win->window);
        return;
    }
    
    if (strlen(nickText) > 20) {
        showError(ERR_NICK_TOO_LONG, win->window);
        return;
    }
    
    // //Uzstāda "loading" kursoru
    GdkDisplay* display = gtk_widget_get_display (widget);
    GdkWindow* window = gtk_widget_get_parent_window(widget);
    GdkCursor* cursor = gdk_cursor_new_for_display(display, GDK_WATCH);
    gdk_window_set_cursor(window, cursor);
    
    // //Izveido servera adreses struktūru
    memset(win->serverAddress, 0, sizeof(win->serverAddress));
    win->serverAddress->sin_family = AF_INET;        //Internet/IP
    win->serverAddress->sin_addr.s_addr = address;   //IP address
    win->serverAddress->sin_port = port;             //Port
    
    //Mēģina pieslēgties kādai spēlei un apstrādā rezultātu
    int32_t id = tryConnect(win);
    switch (id) {
        case -1:
            showError(ERR_NICK_TAKEN, win->window);
            break;
            
        case -2:
            showError(ERR_SERVER_FULL, win->window);
            break;
            
        case -3:
            showError(ERR_CONNECTION, win->window);
            break;
            
        case -4:
            showError(ERR_CONNECTION, win->window);
            break;
    }
    
    //Izdevās pieslēgties
    if (id > 0) {
        win->player->id = id;
        win->player->nick = malloc(strlen(nickText) + 1);
        strcpy(win->player->nick, nickText);
        win->loggedIn = true;
        
        //Aizver pieslēgšanās logu un izit no gtk_main loop
        gtk_widget_destroy(GTK_WIDGET(win->window));
        gtk_main_quit();
    }
    
    //Uzstāda atpakaļ normālo kursoru
    cursor = gdk_cursor_new_for_display(display, GDK_ARROW);
    gdk_window_set_cursor(window, cursor);
}

static void closeWindowClicked(GtkWidget* widget, gpointer data) {
    gtk_main_quit();
}

void login_waitForResult(login_window* window) {
    //No gtk_main atgriežas, pēc gtk_main_quit izsaukuma, kurš tiek izsaukts
    //  aizverot logu vai veiksmīgi pieslēdzoties serverim
    gtk_main();
}

login_window* login_createWindow(int* argc,
                                 char** argv[],
                                 sockets_t* sock,
                                 struct sockaddr_in* serverAddress,
                                 Player* player) {
    login_window* loginWindow = malloc(sizeof(login_window));
    loginWindow->loggedIn = false;
    loginWindow->sock = sock;
    loginWindow->serverAddress = serverAddress;
    loginWindow->player = player;
    
    gtk_init(argc, argv);

    //Izveido jaunu logu un uzstāda nosaukumu
    GtkWidget* window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Connect to server");
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    loginWindow->window = window;

    //Izveido režģi un pievieno to logam
    GtkWidget* grid = gtk_grid_new ();
    gtk_container_add (GTK_CONTAINER (window), grid);
    loginWindow->grid = grid;

    //Uzstāda adreses lauku
    GtkWidget* addressLabel = gtk_label_new("Address:");
    GtkWidget* addressInput = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), addressLabel, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), addressInput, 1, 0, 1, 1);
    loginWindow->addressLabel = addressLabel;
    loginWindow->addressInput = addressInput;
    #if DEBUG
    gtk_entry_set_text(GTK_ENTRY(addressInput), "127.0.0.1");
    #endif

    //Uzstāda porta lauku
    GtkWidget* portLabel = gtk_label_new("Port:");
    GtkWidget* portInput = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), portLabel, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), portInput, 1, 1, 1, 1);
    loginWindow->portLabel = portLabel;
    loginWindow->portInput = portInput;
    #if DEBUG
    gtk_entry_set_text(GTK_ENTRY(portInput), "90");
    #endif
    
    //Uzstāda nika ievadlauku
    GtkWidget* nickLabel = gtk_label_new("Nick:");
    GtkWidget* nickInput = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), nickLabel, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), nickInput, 1, 2, 1, 1);
    loginWindow->nickLabel = nickLabel;
    loginWindow->nickInput = nickInput;
    #if DEBUG
    gtk_entry_set_text(GTK_ENTRY(nickInput), "user");
    #endif
    
    //Uzstāda pogu un tās callback funkciju
    GtkWidget* button = gtk_button_new_with_label ("Connect");
    gtk_grid_attach (GTK_GRID (grid), button, 0, 3, 2, 1);
    
    //Kad nospiež pogu "Connect"
    g_signal_connect (
        button,
        "clicked",
        G_CALLBACK (connectClicked),
        loginWindow
    );
    loginWindow->button = button;

    //Kad nospiež loga "X" pogu
    g_signal_connect(
        window,
        "delete_event",
        G_CALLBACK(closeWindowClicked),
        NULL
    );
    
    return loginWindow;
}

void login_showWindow(login_window* window) {
    gtk_widget_show_all(window->window);
}