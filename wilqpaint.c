#include <gtk/gtk.h>
#include "wilqpaintwin.h"


static void initializeApp(GtkApplication* app, const char *fname)
{
    GtkBuilder *menuBld;
    GMenuModel *menuBar;

    menuBld = gtk_builder_new_from_resource(
            "/org/rafaello7/wilqpaint/menubar.ui");
    menuBar = G_MENU_MODEL(gtk_builder_get_object(menuBld, "menuBar"));
    gtk_application_set_menubar(app, menuBar);
    g_object_unref(menuBld);

    wilqpaint_windowNew(app, fname);
}

static void on_app_activate(GtkApplication* app, gpointer user_data)
{
    initializeApp(app, NULL);
}

void on_app_open(GtkApplication *app, gpointer files,
               gint n_files, gchar *hint, gpointer user_data)
{
    GFile **gfiles = files;
    char *fname = g_file_get_path(gfiles[0]);

    initializeApp(app, fname);
    g_free(fname);
}

int main(int argc, char *argv[])
{
    GtkApplication *app;
    int status;

    app = gtk_application_new (NULL, G_APPLICATION_HANDLES_OPEN);
    g_signal_connect (app, "activate", G_CALLBACK (on_app_activate), NULL);
    g_signal_connect (app, "open", G_CALLBACK (on_app_open), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);
    return status;
}

