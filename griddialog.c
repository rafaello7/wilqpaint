#include <gtk/gtk.h>
#include "griddialog.h"
#include <math.h>

static gdouble gridScale = 8, gridXOffset = 0, gridYOffset = 0;

static void (*onChangeFun)(void);

static void on_scale_change(GtkSpinButton *spin, gpointer user_data)
{
    GtkSpinButton **offsetSpins = user_data;
    GtkAdjustment *adj;
    gdouble offset;
    int i;

    gridScale = gtk_spin_button_get_value(spin);
    for(i = 0; i < 2; ++i) {
        adj = gtk_spin_button_get_adjustment(offsetSpins[i]);
        offset = gtk_adjustment_get_value(adj);
        if( offset >= gridScale )
            gtk_adjustment_set_value(adj, gridScale-1);
        gtk_adjustment_set_upper(adj, gridScale-1);
    }
    onChangeFun();
}

static void on_scale_offset_change(GtkSpinButton *spin, gpointer user_data)
{
    gdouble *param = user_data;

    *param = gtk_spin_button_get_value(spin);
    onChangeFun();
}

void grid_showDialog(GtkWindow *owner, void (*onChange)(void))
{
    GtkBuilder *builder;
    GtkDialog *dialog;
    GtkSpinButton *scaleSpin, *offsetSpins[2];

    onChangeFun = onChange;
    builder = gtk_builder_new_from_resource(
            "/org/rafaello7/wilqpaint/griddialog.ui");
    dialog = GTK_DIALOG(gtk_builder_get_object(builder, "gridDialog"));
    scaleSpin = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "scale"));
    offsetSpins[0] = GTK_SPIN_BUTTON(gtk_builder_get_object(builder,
                "xoffset"));
    offsetSpins[1] = GTK_SPIN_BUTTON(gtk_builder_get_object(builder,
                "yoffset"));
    gtk_spin_button_set_range(offsetSpins[0], 0, gridScale-1);
    gtk_spin_button_set_range(offsetSpins[1], 0, gridScale-1);
    gtk_spin_button_set_value(scaleSpin, gridScale);
    gtk_spin_button_set_value(offsetSpins[0], gridXOffset);
    gtk_spin_button_set_value(offsetSpins[1], gridYOffset);
    g_signal_connect(G_OBJECT(scaleSpin), "value-changed",
            G_CALLBACK(on_scale_change), offsetSpins);
    g_signal_connect(G_OBJECT(offsetSpins[0]), "value-changed",
            G_CALLBACK(on_scale_offset_change), &gridXOffset);
    g_signal_connect(G_OBJECT(offsetSpins[1]), "value-changed",
            G_CALLBACK(on_scale_offset_change), &gridYOffset);
    g_object_unref(builder);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), owner);
    gtk_dialog_run(dialog);
    gtk_widget_destroy(GTK_WIDGET(dialog));
    onChangeFun = NULL;
}

gdouble grid_getScale(void)
{
    return gridScale;
}

gdouble grid_getXOffset(void)
{
    return gridXOffset;
}

gdouble grid_getYOffset(void)
{
    return gridYOffset;
}

gdouble grid_getSnapXValue(gdouble val)
{
    return floor((val + 0.5 * gridScale - gridXOffset) / gridScale) * gridScale
        + gridXOffset;
}

gdouble grid_getSnapYValue(gdouble val)
{
    return floor((val + 0.5 * gridScale - gridYOffset) / gridScale) * gridScale
        + gridYOffset;
}

