#include <gtk/gtk.h>
#include "griddialog.h"
#include <math.h>


struct GridOptions {
    gdouble gridScale;
    gdouble gridXOffset;
    gdouble gridYOffset;
    gboolean showGrid;
    gboolean snapToGrid;
};

struct GridCallbackParam {
    GridOptions *opts;
    GtkWindow *owner;
    void (*onChangeFun)(GtkWindow*);
    GtkSpinButton *spinXOffset;
    GtkSpinButton *spinYOffset;
};

GridOptions *grid_optsNew(void)
{
    GridOptions *opts;

    opts = g_malloc(sizeof(struct GridOptions));
    opts->gridScale = 8;
    opts->gridXOffset = 0;
    opts->gridYOffset = 0;
    opts->showGrid = FALSE;
    opts->snapToGrid = FALSE;
    return opts;
}

static void on_scale_change(GtkSpinButton *spin, gpointer user_data)
{
    struct GridCallbackParam *par = user_data;
    GtkAdjustment *adj;
    gdouble offset;

    par->opts->gridScale = gtk_spin_button_get_value(spin);
    adj = gtk_spin_button_get_adjustment(par->spinXOffset);
    offset = gtk_adjustment_get_value(adj);
    if( offset >= par->opts->gridScale )
        gtk_adjustment_set_value(adj, par->opts->gridScale-1);
    gtk_adjustment_set_upper(adj, par->opts->gridScale-1);
    adj = gtk_spin_button_get_adjustment(par->spinYOffset);
    offset = gtk_adjustment_get_value(adj);
    if( offset >= par->opts->gridScale )
        gtk_adjustment_set_value(adj, par->opts->gridScale-1);
    gtk_adjustment_set_upper(adj, par->opts->gridScale-1);
    par->onChangeFun(par->owner);
}

static void on_scale_xoffset_change(GtkSpinButton *spin, gpointer user_data)
{
    struct GridCallbackParam *par = user_data;

    par->opts->gridXOffset = gtk_spin_button_get_value(spin);
    par->onChangeFun(par->owner);
}

static void on_scale_yoffset_change(GtkSpinButton *spin, gpointer user_data)
{
    struct GridCallbackParam *par = user_data;

    par->opts->gridYOffset = gtk_spin_button_get_value(spin);
    par->onChangeFun(par->owner);
}

static void on_showGrid_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    struct GridCallbackParam *par = user_data;

    par->opts->showGrid = gtk_toggle_button_get_active(toggle);
    par->onChangeFun(par->owner);
}

static void on_snapToGrid_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    struct GridCallbackParam *par = user_data;

    par->opts->snapToGrid = gtk_toggle_button_get_active(toggle);
    par->onChangeFun(par->owner);
}

void grid_showDialog(GridOptions *opts, GtkWindow *owner,
        void (*onChange)(GtkWindow*))
{
    GtkBuilder *builder;
    GtkDialog *dialog;
    GtkSpinButton *scaleSpin;
    GtkToggleButton *showGridBtn, *snapToGridBtn;
    struct GridCallbackParam par;

    par.opts = opts;
    par.owner = owner;
    par.onChangeFun = onChange;
    builder = gtk_builder_new_from_resource(
            "/org/rafaello7/wilqpaint/griddialog.ui");
    dialog = GTK_DIALOG(gtk_builder_get_object(builder, "gridDialog"));
    scaleSpin = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "scale"));
    par.spinXOffset = GTK_SPIN_BUTTON(gtk_builder_get_object(builder,
                "xoffset"));
    par.spinYOffset = GTK_SPIN_BUTTON(gtk_builder_get_object(builder,
                "yoffset"));
    showGridBtn = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder,
                "showGrid"));
    snapToGridBtn = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder,
                "snapToGrid"));
    gtk_spin_button_set_range(par.spinXOffset, 0, opts->gridScale-1);
    gtk_spin_button_set_range(par.spinYOffset, 0, opts->gridScale-1);
    gtk_spin_button_set_value(scaleSpin, opts->gridScale);
    gtk_spin_button_set_value(par.spinXOffset, opts->gridXOffset);
    gtk_spin_button_set_value(par.spinYOffset, opts->gridYOffset);
    gtk_toggle_button_set_active(showGridBtn, opts->showGrid);
    gtk_toggle_button_set_active(snapToGridBtn, opts->snapToGrid);
    g_signal_connect(G_OBJECT(scaleSpin), "value-changed",
            G_CALLBACK(on_scale_change), &par);
    g_signal_connect(G_OBJECT(par.spinXOffset), "value-changed",
            G_CALLBACK(on_scale_xoffset_change), &par);
    g_signal_connect(G_OBJECT(par.spinXOffset), "value-changed",
            G_CALLBACK(on_scale_yoffset_change), &par);
    g_signal_connect(G_OBJECT(showGridBtn), "toggled",
            G_CALLBACK(on_showGrid_toggled), &par);
    g_signal_connect(G_OBJECT(snapToGridBtn), "toggled",
            G_CALLBACK(on_snapToGrid_toggled), &par);
    g_object_unref(builder);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), owner);
    gtk_dialog_run(dialog);
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

gdouble grid_getScale(GridOptions *opts)
{
    return opts->gridScale;
}

gdouble grid_getXOffset(GridOptions *opts)
{
    return opts->gridXOffset;
}

gdouble grid_getYOffset(GridOptions *opts)
{
    return opts->gridYOffset;
}

gdouble grid_getSnapXValue(GridOptions *opts, gdouble val)
{
    return floor((val + 0.5 * opts->gridScale - opts->gridXOffset)
            / opts->gridScale) * opts->gridScale + opts->gridXOffset;
}

gdouble grid_getSnapYValue(GridOptions *opts, gdouble val)
{
    return floor((val + 0.5 * opts->gridScale - opts->gridYOffset)
            / opts->gridScale) * opts->gridScale + opts->gridYOffset;
}

gboolean grid_isShow(GridOptions *opts)
{
    return opts->showGrid;
}

gboolean grid_isSnapTo(GridOptions *opts)
{
    return opts->snapToGrid;
}

void grid_setIsShow(GridOptions *opts, gboolean isShow)
{
    opts->showGrid = isShow;
}

void grid_setIsSnapTo(GridOptions *opts, gboolean isSnapTo)
{
    opts->snapToGrid = isSnapTo;
}

void grid_optsFree(GridOptions *opts)
{
    g_free(opts);
}

