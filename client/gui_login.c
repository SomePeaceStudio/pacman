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
#define ERR_CONNECTION "Failed to connect to server"

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
bool try_connect(in_addr_t address, uint16_t port, const char* nick);
//Parāda kļūdas paziņojumu. widget ir nepieciešams, lai atrasu GtkWindow
void show_error(const char* message, GtkWidget* widget);

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
    
    //Uzstāda "loading" kursoru
    GdkDisplay* display = gtk_widget_get_display (widget);
    GdkWindow* window = gtk_widget_get_parent_window(widget);
    GdkCursor* cursor = gdk_cursor_new_for_display(display, GDK_WATCH);
    gdk_window_set_cursor(window, cursor);
    
    // if (try_connect(address, port, nickText)) {
    //     printf("Success\n");
    // } else {
    //     show_error(ERR_CONNECT, form->portInput);
    // }
    
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