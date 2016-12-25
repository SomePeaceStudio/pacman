#include <gtk/gtk.h>
#include <stdio.h>

static void print_hello (GtkWidget* widget, gpointer data) {
  printf ("Hello World\n");
}

static void activate (GtkApplication *app, gpointer user_data) {
    GtkWidget* window;
    GtkWidget* grid;
    GtkWidget* addressInput;
    GtkWidget* addressLabel;
    GtkWidget* portInput;
    GtkWidget* portLabel;
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
    
    //Uzstāda pogu
    button = gtk_button_new_with_label ("Connect");
    gtk_grid_attach (GTK_GRID (grid), button, 0, 2, 2, 1);
    g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);

    //Parāda visu
    gtk_widget_show_all (window);
}

int main (int argc, char *argv[]) {
    GtkApplication *app;
    int status;

    app = gtk_application_new ("lv.lsp.pacman", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}