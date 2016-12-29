#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>

#include "../shared/shared.h"
#include "login.h"
#include "globals.h"

//Teksti, ko rādīt lietotājam
#define ERR_ADDRESS "Please enter a valid IP address"
#define ERR_PORT "Please enter a valid port number"
#define ERR_NICK_EMPTY "Please enter a nick"
#define ERR_NICK_TAKEN "Chosen nick is already taken"
#define ERR_NICK_TOO_LONG "Nick is too long (max: 20)"
#define ERR_CONNECTION "Failed to connect to server"
#define ERR_SERVER_FULL "Server is full"

//Struktūra, lai padotu visus ievadformas datus pogas callback funkcijai
typedef struct {
    GtkWidget* addressInput;
    GtkWidget* portInput;
    GtkWidget* nickInput;
    GtkWindow* window;
} InputForm;

//Nosaka, vai ir izdevies pieslēgties kādai spēlei, vai ne
bool gLoggedIn = false;

//Parāda kļūdas paziņojumu lietotājam.
//widget (var būt NULL): lai noteiktu vecāka logu (GtkWindow)
void showError(const char* message, GtkWindow* window) {
    //Izveido un parāda dialogu
    GtkWidget* dialog = gtk_message_dialog_new(
        window,             //Dialoga vecāks (logs)
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

//Atgriež spēlētāja id, vai kļūdas kodu:
//-1: niks aizņemts
//-2: serveris pilns
//-3: nepareiza / negaidīta atbilde no servera
int32_t joinGame(int sock, const char* playerName) {
    unsigned char* pack;

    //1. baits paketes tipam
    pack = allocPack(PSIZE_JOIN);
    pack[0] = PTYPE_JOIN;
    debug_print("Size of name: %ld\n", sizeof(playerName));
    memcpy(&pack[1], playerName, strlen(playerName));

    // Send JOIN packet
    debug_print("%s\n", "Sending JOIN packet...");
    safeSend(sock, pack, PSIZE_JOIN, 0);
    free(pack);

    // Receive ACK
    pack = allocPack(PSIZE_ACK);
    
    safeRecv(sock, pack, PSIZE_ACK, 0);
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

//Argriež spēlētāja id vai kļūdas paziņojumu no joinGame() vai:
//-4: ja neizdevās savienoties ar serveri
int tryConnect(in_addr_t address, uint16_t port, const char* nick) {
    int sock;
    struct sockaddr_in serverAddress;
    
    /* Create the TCP socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        Die(ERR_SOCKET);
    }
    
    /* Construct the server sockaddr_in structure */
    memset(&serverAddress, 0, sizeof(serverAddress));  /* Clear struct */
    serverAddress.sin_family = AF_INET;                /* Internet/IP */
    serverAddress.sin_addr.s_addr = address;           /* IP address */
    serverAddress.sin_port = port;                     /* server port */
    
    //Try to establish connection
    if (connect(sock,
                (struct sockaddr *) &serverAddress,
                sizeof(serverAddress)) < 0) {
        return -4;
    }
    
    return joinGame(sock, nick);
}

//Callback funkcija, kad tiek nospiesta poga "Connect"
static void connectClicked (GtkWidget* widget, gpointer data) {
    InputForm* form = (InputForm*)data;
    // 
    // //Dabū ievadītās vērtības no formas
    const char* addressText = gtk_entry_get_text(GTK_ENTRY(form->addressInput));
    const char* portText = gtk_entry_get_text(GTK_ENTRY(form->portInput));
    const char* nickText = gtk_entry_get_text(GTK_ENTRY(form->nickInput));
    
    in_addr_t address = inet_addr(addressText);
    uint16_t port = htons(atoi(portText));
    
    if (address == INADDR_NONE) {
        //Nav svarīgi, kādu GtkWidget padod showError funkcijai
        showError(ERR_ADDRESS, form->window);
        return;
    }
    
    if (strlen(portText) == 0 || port == 0) {
        showError(ERR_PORT, form->window);
        return;
    }
    
    if (strlen(nickText) == 0) {
        showError(ERR_NICK_EMPTY, form->window);
        return;
    }
    
    if (strlen(nickText) > 20) {
        showError(ERR_NICK_TOO_LONG, form->window);
        return;
    }
    
    //Uzstāda "loading" kursoru
    GdkDisplay* display = gtk_widget_get_display (widget);
    GdkWindow* window = gtk_widget_get_parent_window(widget);
    GdkCursor* cursor = gdk_cursor_new_for_display(display, GDK_WATCH);
    gdk_window_set_cursor(window, cursor);
    
    //Mēģģina pieslēgties kādai spēlei un apstrādā rezultātu
    int id = tryConnect(address, port, nickText);
    switch (id) {
        case -1:
            showError(ERR_NICK_TAKEN, form->window);
            break;
            
        case -2:
            showError(ERR_SERVER_FULL, form->window);
            break;
            
        case -3:
            showError(ERR_CONNECTION, form->window);
            break;
            
        case -4:
            showError(ERR_CONNECTION, form->window);
            break;
    }
    
    //Izdevās pieslēgties
    if (id > 0) {
        //Uzstādam globālos mainīgos
        gPlayerId = id;
        gPlayerName = malloc (strlen(nickText) + 1);
        strcpy(gPlayerName, nickText);
        gLoggedIn = true;
        
        //Aizver pieslēgšanās logu un izit no gtk_main loop
        gtk_widget_destroy(GTK_WIDGET(form->window));
        gtk_main_quit();
    }
    
    //Uzstāda atpakaļ normālo kursoru
    cursor = gdk_cursor_new_for_display(display, GDK_ARROW);
    gdk_window_set_cursor(window, cursor);
}

static void closeWindowClicked(GtkWidget* widget, gpointer data) {
    gtk_main_quit();
}

bool showLoginForm(int* argc, char** argv[]) {
    GtkWidget* window;
    GtkWidget* grid;
    GtkWidget* addressInput;
    GtkWidget* portInput;
    GtkWidget* nickInput;
    GtkWidget* addressLabel;
    GtkWidget* portLabel;
    GtkWidget* nickLabel;
    GtkWidget* button;

    gtk_init(argc, argv);

    //Izveido jaunu logu un uzstāda nosaukumu
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Connect to server");
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);

    //Izveido režģi un pievieno to logam
    grid = gtk_grid_new ();
    gtk_container_add (GTK_CONTAINER (window), grid);

    //Uzstāda adreses lauku
    addressLabel = gtk_label_new("Address:");
    addressInput = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), addressLabel, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), addressInput, 1, 0, 1, 1);
    #if DEBUG
    gtk_entry_set_text(GTK_ENTRY(addressInput), "127.0.0.1");
    #endif

    //Uzstāda porta lauku
    portLabel = gtk_label_new("Port:");
    portInput = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), portLabel, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), portInput, 1, 1, 1, 1);
    #if DEBUG
    gtk_entry_set_text(GTK_ENTRY(portInput), "90");
    #endif
    
    //Uzstāda nika ievadlauku
    nickLabel = gtk_label_new("Nick:");
    nickInput = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), nickLabel, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), nickInput, 1, 2, 1, 1);
    #if DEBUG
    gtk_entry_set_text(GTK_ENTRY(nickInput), "user");
    #endif
    
    //Izveido ivaddatu struktūru, ko padot callback funkcijai
    InputForm* userInput = malloc(sizeof(InputForm));
    if (userInput == NULL) {
        Die(ERR_MALLOC);
    }
    userInput->addressInput = addressInput;
    userInput->portInput = portInput;
    userInput->nickInput = nickInput;
    userInput->window = GTK_WINDOW(window);
    
    //Uzstāda pogu un tās callback funkciju
    button = gtk_button_new_with_label ("Connect");
    gtk_grid_attach (GTK_GRID (grid), button, 0, 3, 2, 1);
    g_signal_connect (
        button,
        "clicked",
        G_CALLBACK (connectClicked),
        userInput
    );
    
    g_signal_connect(
        window,
        "delete_event",
        G_CALLBACK(closeWindowClicked),
        NULL
    );

    gtk_widget_show_all (window);
    
    gtk_main();
    
    return gLoggedIn;
}