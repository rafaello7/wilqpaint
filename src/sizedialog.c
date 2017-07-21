#include <gtk/gtk.h>
#include "sizedialog.h"

static gdouble ratio;
static GtkSpinButton *spinWidth, *spinHeight;
static gboolean isAutoChange = FALSE;

static void on_spinwidth_changed(GtkSpinButton *spin, gpointer user_data)
{
    gdouble width;

    if( ! isAutoChange ) {
        isAutoChange = TRUE;
        width = gtk_spin_button_get_value(spinWidth);
        gtk_spin_button_set_value(spinHeight, width * ratio);
        isAutoChange = FALSE;
    }
}

static void on_spinheight_changed(GtkSpinButton *spin, gpointer user_data)
{
    gdouble height;

    if( ! isAutoChange ) {
        isAutoChange = TRUE;
        height = gtk_spin_button_get_value(spinHeight);
        gtk_spin_button_set_value(spinWidth, height / ratio);
        isAutoChange = FALSE;
    }
}

static void on_buttonx2_changed(GtkButton *button, gpointer user_data)
{
    gdouble val;

	isAutoChange = TRUE;
	val = gtk_spin_button_get_value(spinWidth);
	gtk_spin_button_set_value(spinWidth, 2.0 * val);
	val = gtk_spin_button_get_value(spinHeight);
	gtk_spin_button_set_value(spinHeight, 2.0 * val);
	isAutoChange = FALSE;
}

static void on_buttondiv2_changed(GtkButton *button, gpointer user_data)
{
    gdouble val;

	isAutoChange = TRUE;
	val = gtk_spin_button_get_value(spinWidth);
	gtk_spin_button_set_value(spinWidth, 0.5 * val);
	val = gtk_spin_button_get_value(spinHeight);
	gtk_spin_button_set_value(spinHeight, 0.5 * val);
	isAutoChange = FALSE;
}

gboolean showSizeDialog(GtkWindow *owner, const char *title,
        gdouble *width, gdouble *height, gboolean keepRatio)
{
    GtkBuilder *builder;
    GtkDialog *dialog;
    gboolean result = FALSE;
	GtkButton *buttonX2, *buttonDiv2;

    builder = gtk_builder_new_from_resource(
            "/org/rafaello7/wilqpaint/sizedialog.ui");
    dialog = GTK_DIALOG(gtk_builder_get_object(builder, "sizeDialog"));
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    spinWidth = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinWidth"));
    spinHeight = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinHeight"));
    buttonX2 = GTK_BUTTON(gtk_builder_get_object(builder, "buttonX2"));
    buttonDiv2 = GTK_BUTTON(gtk_builder_get_object(builder, "buttonDiv2"));
    g_object_unref(builder);
    gtk_spin_button_set_value(spinWidth, *width);
    gtk_spin_button_set_value(spinHeight, *height);
    if( keepRatio ) {
        ratio = 1.0 * *height / *width;
        g_signal_connect(G_OBJECT(spinWidth), "value-changed",
            G_CALLBACK(on_spinwidth_changed), &ratio);
        g_signal_connect(G_OBJECT(spinHeight), "value-changed",
            G_CALLBACK(on_spinheight_changed), &ratio);
    }
	g_signal_connect(G_OBJECT(buttonX2), "clicked",
		G_CALLBACK(on_buttonx2_changed), NULL);
	g_signal_connect(G_OBJECT(buttonDiv2), "clicked",
		G_CALLBACK(on_buttondiv2_changed), NULL);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), owner);
    if( gtk_dialog_run(dialog) == 1 ) {
        *width = gtk_spin_button_get_value(spinWidth);
        *height = gtk_spin_button_get_value(spinHeight);
        result = TRUE;
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
    return result;
}
