#include <gtk/gtk.h>
#include "quitdialog.h"

gint showQuitDialog(GtkWindow *owner)
{
    GtkBuilder *builder;
    GtkDialog *dialog;
    gint result;

    builder = gtk_builder_new_from_resource(
            "/org/rafaello7/wilqpaint/quitdialog.ui");
    dialog = GTK_DIALOG(gtk_builder_get_object(builder, "quitDialog"));
    g_object_unref(builder);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), owner);
    switch( gtk_dialog_run(dialog) ) {
    case 0:
        result = GTK_RESPONSE_YES;
        break;
    case 1:
        result = GTK_RESPONSE_NO;
        break;
    case 2:
        result = GTK_RESPONSE_CANCEL;
        break;
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
    return result;
}
