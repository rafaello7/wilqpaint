#include <gtk/gtk.h>
#include "thresholddialog.h"


struct ValueChangeHandlerParam {
    GtkWindow *owner;
    void (*onChange)(GtkWindow*, gdouble);
};

void on_thresholdLevelAdjustment_valuechanged(GtkAdjustment *adjustment,
               gpointer user_data)
{
    struct ValueChangeHandlerParam *par = user_data;

    par->onChange(par->owner, 0.01 * gtk_adjustment_get_value(adjustment));
}

gboolean showThresholdDialog(GtkWindow *owner, gdouble initialValue,
        void (*onChange)(GtkWindow*, gdouble))
{
    GtkBuilder *builder;
    GtkDialog *dialog;
    GtkAdjustment *thresholdLevelAdjustment;
    gboolean result = FALSE;
    struct ValueChangeHandlerParam par;

    builder = gtk_builder_new_from_resource(
            "/org/rafaello7/wilqpaint/thresholddialog.ui");
    dialog = GTK_DIALOG(gtk_builder_get_object(builder, "thresholdDialog"));
    thresholdLevelAdjustment = GTK_ADJUSTMENT(gtk_builder_get_object(builder,
                "thresholdLevelAdjustment"));
    gtk_adjustment_set_value(thresholdLevelAdjustment, 100.0 * initialValue);
    g_object_unref(builder);
    par.owner = owner;
    par.onChange = onChange;
    g_signal_connect(thresholdLevelAdjustment, "value-changed",
            G_CALLBACK(on_thresholdLevelAdjustment_valuechanged), &par);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), owner);
    if( gtk_dialog_run(dialog) == GTK_RESPONSE_ACCEPT )
        result = TRUE;
    gtk_widget_destroy(GTK_WIDGET(dialog));
    return result;
}
