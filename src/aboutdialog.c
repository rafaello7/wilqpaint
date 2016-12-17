#include <gtk/gtk.h>
#include "aboutdialog.h"

void showAboutDialog(GtkWindow *owner)
{
    GtkBuilder *builder;
    GtkDialog *dialog;
    gint result;

    builder = gtk_builder_new_from_resource(
            "/org/rafaello7/wilqpaint/aboutdialog.ui");
    dialog = GTK_DIALOG(gtk_builder_get_object(builder, "aboutDialog"));
    g_object_unref(builder);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), owner);
    gtk_dialog_run(dialog);
    gtk_widget_destroy(GTK_WIDGET(dialog));
}
