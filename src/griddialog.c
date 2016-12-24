#include <gtk/gtk.h>
#include "griddialog.h"
#include <math.h>


enum SnapDimension {
    SD_0PX,
    SD_HALFPX,
    SD_1PX_CORNER,
    SD_1PX_CENTER,
    SD_TOGRID,
    SD_COUNT
};

static const char gSnapIds[][12] = {
    "0px", "0.5px", "1pxCorner", "1pxCenter", "toGrid"
};

struct GridOptions {
    gdouble gridScale;
    gdouble gridXOffset;
    gdouble gridYOffset;
    gboolean showGrid;
    enum SnapDimension snapDim;
    gboolean isPixelHitOnCenter;
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
    opts->gridScale = 8.0;
    opts->gridXOffset = 0.5;
    opts->gridYOffset = 0.5;
    opts->showGrid = FALSE;
    opts->snapDim = SD_1PX_CENTER;
    opts->isPixelHitOnCenter = TRUE;
    return opts;
}

static void on_showGrid_notify_active(GObject *obj, GParamSpec *pspec,
        gpointer user_data)
{
    struct GridCallbackParam *par = user_data;

    par->opts->showGrid = gtk_switch_get_active(GTK_SWITCH(obj));
    par->onChangeFun(par->owner);
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
        gtk_adjustment_set_value(adj, par->opts->gridScale - 0.5);
    gtk_adjustment_set_upper(adj, par->opts->gridScale - 0.5);
    adj = gtk_spin_button_get_adjustment(par->spinYOffset);
    offset = gtk_adjustment_get_value(adj);
    if( offset >= par->opts->gridScale )
        gtk_adjustment_set_value(adj, par->opts->gridScale - 0.5);
    gtk_adjustment_set_upper(adj, par->opts->gridScale - 0.5);
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

void grid_showDialog(GridOptions *opts, GtkWindow *owner,
        void (*onChange)(GtkWindow*))
{
    GtkBuilder *builder;
    GtkDialog *dialog;
    GtkSpinButton *scaleSpin;
    GtkSwitch *showGridBtn;
    GtkToggleButton *snapNoneBtn, *snap05pxBtn;
    GtkToggleButton *snap1pxCornerBtn, *snap1pxCenterBtn, *snapToGridBtn;
    GtkToggleButton *hitCornerBtn, *hitCenterBtn, *activeBtn;
    struct GridCallbackParam par;

    par.opts = opts;
    par.owner = owner;
    par.onChangeFun = onChange;
    builder = gtk_builder_new_from_resource(
            "/org/rafaello7/wilqpaint/griddialog.ui");
    dialog = GTK_DIALOG(gtk_builder_get_object(builder, "gridDialog"));
    showGridBtn = GTK_SWITCH(gtk_builder_get_object(builder, "showGrid"));
    scaleSpin = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "scale"));
    par.spinXOffset = GTK_SPIN_BUTTON(gtk_builder_get_object(builder,
                "xoffset"));
    par.spinYOffset = GTK_SPIN_BUTTON(gtk_builder_get_object(builder,
                "yoffset"));
    snapNoneBtn = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder,
                "snapNone"));
    snap05pxBtn = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder,
                "snap05px"));
    snap1pxCornerBtn = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder,
                "snap1pxCorner"));
    snap1pxCenterBtn = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder,
                "snap1pxCenter"));
    snapToGridBtn = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder,
                "snapToGrid"));
    hitCornerBtn = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder,
                "hitCorner"));
    hitCenterBtn = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder,
                "hitCenter"));
    gtk_switch_set_active(showGridBtn, opts->showGrid);
    gtk_spin_button_set_range(par.spinXOffset, 0, opts->gridScale-0.5);
    gtk_spin_button_set_range(par.spinYOffset, 0, opts->gridScale-0.5);
    gtk_spin_button_set_value(scaleSpin, opts->gridScale);
    gtk_spin_button_set_value(par.spinXOffset, opts->gridXOffset);
    gtk_spin_button_set_value(par.spinYOffset, opts->gridYOffset);
    switch( opts->snapDim ) {
    case SD_0PX:
        activeBtn = snapNoneBtn;
        break;
    case SD_HALFPX:
        activeBtn = snap05pxBtn;
        break;
    case SD_1PX_CORNER:
        activeBtn = snap1pxCornerBtn;
        break;
    case SD_TOGRID:
        activeBtn = snapToGridBtn;
        break;
    default:
        activeBtn = snap1pxCenterBtn;
        break;
    }
    gtk_toggle_button_set_active(activeBtn, TRUE);
    gtk_toggle_button_set_active(
            opts->isPixelHitOnCenter ? hitCenterBtn : hitCornerBtn, TRUE);
    g_signal_connect(G_OBJECT(showGridBtn), "notify::active",
            G_CALLBACK(on_showGrid_notify_active), &par);
    g_signal_connect(G_OBJECT(scaleSpin), "value-changed",
            G_CALLBACK(on_scale_change), &par);
    g_signal_connect(G_OBJECT(par.spinXOffset), "value-changed",
            G_CALLBACK(on_scale_xoffset_change), &par);
    g_signal_connect(G_OBJECT(par.spinYOffset), "value-changed",
            G_CALLBACK(on_scale_yoffset_change), &par);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), owner);
    gtk_dialog_run(dialog);
    gtk_widget_destroy(GTK_WIDGET(dialog));
    if( gtk_toggle_button_get_active(snapNoneBtn) )
        opts->snapDim = SD_0PX;
    else if( gtk_toggle_button_get_active(snap05pxBtn) )
        opts->snapDim = SD_HALFPX;
    else if( gtk_toggle_button_get_active(snap1pxCornerBtn) )
        opts->snapDim = SD_1PX_CORNER;
    else if( gtk_toggle_button_get_active(snap1pxCenterBtn) )
        opts->snapDim = SD_1PX_CENTER;
    else if( gtk_toggle_button_get_active(snapToGridBtn) )
        opts->snapDim = SD_TOGRID;
    opts->isPixelHitOnCenter = gtk_toggle_button_get_active(hitCenterBtn);
    g_object_unref(builder);
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

static gdouble getSnapValue(GridOptions *opts,
        gdouble gridOffset, gdouble val, gdouble zoom)
{
    gdouble res = val;

    switch( opts->snapDim ) {
    case SD_0PX:
        if( zoom <= 1.0 && opts->isPixelHitOnCenter )
            res += 0.5;
        break;
    case SD_HALFPX:
        if( zoom <= 1.0 && opts->isPixelHitOnCenter )
            res += 0.5;
        else if( zoom > 2.0 )
            res = 0.5 * floor(2.0 * res + 0.5);
        break;
    case SD_1PX_CORNER:
        res = floor(res + 0.5);
        break;
    default:    /* SD_1PX_CENTER */
        res = floor(res) + 0.5;
        break;
    case SD_TOGRID:
        res = floor((res + 0.5 * opts->gridScale - gridOffset)
                / opts->gridScale) * opts->gridScale + gridOffset;
        break;
    }
    return res;
}

gdouble grid_getSnapXValue(GridOptions *opts, gdouble val, gdouble zoom)
{
    return getSnapValue(opts, opts->gridXOffset, val, zoom);
}

gdouble grid_getSnapYValue(GridOptions *opts, gdouble val, gdouble zoom)
{
    return getSnapValue(opts, opts->gridYOffset, val, zoom);
}

gboolean grid_isShow(GridOptions *opts)
{
    return opts->showGrid;
}

void grid_setIsShow(GridOptions *opts, gboolean isShow)
{
    opts->showGrid = isShow;
}

const char *grid_getSnapId(GridOptions *opts)
{
    return gSnapIds[opts->snapDim];
}

void grid_setSnapById(GridOptions *opts, const char *snapId)
{
    int i;

    for(i = 0; i < SD_COUNT && g_strcmp0(gSnapIds[i], snapId); ++i) {
    }
    opts->snapDim = i == SD_COUNT ? SD_1PX_CENTER : i;
}

void grid_optsFree(GridOptions *opts)
{
    g_free(opts);
}

