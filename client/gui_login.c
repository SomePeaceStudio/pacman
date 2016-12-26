#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#include "../shared/shared.h"

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
} InputForm;

//Click callback priekš pogas "Connect"
static void connect_clicked (GtkWidget* widget, gpointer data);
//Izveido un parāda visus logrīkus (widgets)
static void activate (GtkApplication *app, gpointer user_data);
//Mēģina pievienoties serverim
int try_connect(in_addr_t address, uint16_t port, const char* nick);
//Parāda kļūdas paziņojumu. widget ir nepieciešams, lai atrasu GtkWindow
void show_error(const char* message, GtkWidget* widget);
int joinGame(int sock, const char* playerName);

int main (int argc, char *argv[]) {
    GtkApplication *app;
    int status;

    app = gtk_application_new ("lv.lsp.pacman", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}


static void connect_clicked (GtkWidget* widget, gpointer data) {
    InputForm* form = (InputForm*)data;
    
    //Dabū ievadītās vērtības no formas
    const char* addressText = gtk_entry_get_text(GTK_ENTRY(form->addressInput));
    const char* portText = gtk_entry_get_text(GTK_ENTRY(form->portInput));
    const char* nickText = gtk_entry_get_text(GTK_ENTRY(form->nickInput));
    
    in_addr_t address = inet_addr(addressText);
    uint16_t port = htons(atoi(portText));
    
    if (address == INADDR_NONE) {
        //Nav svarīgi, kādu GtkWidget padod show_error funkcijai
        show_error(ERR_ADDRESS, form->portInput);
        return;
    }
    
    if (strlen(portText) == 0 || port == 0) {
        show_error(ERR_PORT, form->portInput);
        return;
    }
    
    if (strlen(nickText) == 0) {
        show_error(ERR_NICK_EMPTY, form->portInput);
        return;
    }
    
    if (strlen(nickText) > 20) {
        show_error(ERR_NICK_TOO_LONG, form->portInput);
        return;
    }
    
    //Uzstāda "loading" kursoru
    GdkDisplay* display = gtk_widget_get_display (widget);
    GdkWindow* window = gtk_widget_get_parent_window(widget);
    GdkCursor* cursor = gdk_cursor_new_for_display(display, GDK_WATCH);
    gdk_window_set_cursor(window, cursor);
    
    int id = try_connect(address, port, nickText);
    
    switch (id) {
        case -1:
            show_error(ERR_NICK_TAKEN, form->portInput);
            break;
            
        case -2:
            show_error(ERR_SERVER_FULL, form->portInput);
            break;
            
        case -3:
            show_error(ERR_CONNECTION, form->portInput);
            break;
            
        case -4:
            show_error(ERR_CONNECTION, form->portInput);
            break;
    }
    
    if (id > 0) {
        show_error("CONNECTED!!!!", form->portInput);
    }
    
    //Uzstāda atpakaļ normālo kursoru
    cursor = gdk_cursor_new_for_display(display, GDK_ARROW);
    gdk_window_set_cursor(window, cursor);
}


static void activate (GtkApplication *app, gpointer user_data) {
    GtkWidget* window;
    GtkWidget* grid;
    GtkWidget* addressInput;
    GtkWidget* portInput;
    GtkWidget* nickInput;
    GtkWidget* addressLabel;
    GtkWidget* portLabel;
    GtkWidget* nickLabel;
    GtkWidget* button;

    //Izveido jaunu logu un uzstāda nosaukumu
    window = gtk_application_window_new (app);
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

    //Uzstāda porta lauku
    portLabel = gtk_label_new("Port:");
    portInput = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), portLabel, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), portInput, 1, 1, 1, 1);
    
    //Uzstāda nika ievadlauku
    nickLabel = gtk_label_new("Nick:");
    nickInput = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), nickLabel, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), nickInput, 1, 2, 1, 1);
    
    //Izveido ivaddatu struktūru, ko padot callback funkcijai
    InputForm* userInput = malloc(sizeof(InputForm));
    if (userInput == NULL) {
        Die(ERR_MALLOC);
    }
    userInput->addressInput = addressInput;
    userInput->portInput = portInput;
    userInput->nickInput = nickInput;
    
    //Uzstāda pogu un tās callback funkciju
    button = gtk_button_new_with_label ("Connect");
    gtk_grid_attach (GTK_GRID (grid), button, 0, 3, 2, 1);
    g_signal_connect (button,
                      "clicked",
                      G_CALLBACK (connect_clicked),
                      userInput);

    //Parāda visu
    gtk_widget_show_all (window);
}


//Argriež spēlētāja id vai kļūdas paziņojumu no joinGame() vai:
//-4: ja neizdevās savienoties ar serveri
int try_connect(in_addr_t address, uint16_t port, const char* nick) {
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


//Atgriež spēlētāja id, vai kļūdas kodu:
//-1: niks aizņemts
//-2: serveris pilns
//-3: nepareiza / negaidīta atbilde no servera
int joinGame(int sock, const char* playerName) {
    char* pack;

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
    
    int id;
    if(pack[0] == PTYPE_ACK) {
        id = (int)pack[1];
    } else {
        //Ja atsūtīta nepareiza tipa pakete
        id = -3;
    }
    
    free(pack);
    return id;
}


void show_error(const char* message, GtkWidget* widget) {
    //Dabū window objektu
    GtkWindow* window = NULL;
    GtkWidget* toplevel = gtk_widget_get_toplevel (widget);
    if (gtk_widget_is_toplevel (toplevel))
        window = GTK_WINDOW(toplevel);
    
    //Izveido un parāda dialogu
    GtkWidget* dialog = gtk_message_dialog_new(
        window,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "%s", message
    );
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}