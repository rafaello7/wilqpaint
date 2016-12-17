#include <gtk/gtk.h>
#include "wilqpaintapp.h"
#include "wilqpaintwin.h"


struct WilqpaintApp {
    GtkApplication parent;
};

typedef struct {
    GtkApplicationClass parent_class;
} WilqpaintAppClass;

G_DEFINE_TYPE(WilqpaintApp, wilqpaint_app, GTK_TYPE_APPLICATION);


static void wilqpaint_app_init(WilqpaintApp *app)
{
}

static void wilqpaint_app_startup(GApplication *app)
{
    GtkBuilder *menuBld;
    GMenuModel *menuBar;

    G_APPLICATION_CLASS(wilqpaint_app_parent_class)->startup(app);
    menuBld = gtk_builder_new_from_resource(
            "/org/rafaello7/wilqpaint/menubar.ui");
    menuBar = G_MENU_MODEL(gtk_builder_get_object(menuBld, "menuBar"));
    gtk_application_set_menubar(GTK_APPLICATION(app), menuBar);
    g_object_unref(menuBld);
}

static void wilqpaint_app_activate(GApplication *app)
{
    wilqpaint_windowNew(GTK_APPLICATION(app), NULL);
}

static void wilqpaint_app_open(GApplication  *app,
        GFile **files, gint n_files, const gchar *hint)
{
    int i;

    for (i = 0; i < n_files; i++) {
        char *fname = g_file_get_path(files[i]);
        wilqpaint_windowNew(GTK_APPLICATION(app), fname);
        g_free(fname);
    }
}

static void wilqpaint_app_class_init (WilqpaintAppClass *class)
{
    G_APPLICATION_CLASS(class)->startup = wilqpaint_app_startup;
    G_APPLICATION_CLASS(class)->activate = wilqpaint_app_activate;
    G_APPLICATION_CLASS(class)->open = wilqpaint_app_open;
}

WilqpaintApp *wilqpaint_appNew(void)
{
    return g_object_new (WILQPAINT_APP_TYPE,
            //"application-id", "org.rafaello7.wilqpaint",
            "flags", G_APPLICATION_HANDLES_OPEN,
            NULL);
}

