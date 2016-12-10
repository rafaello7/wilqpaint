#include <gtk/gtk.h>
#include "wilqpaintapp.h"


int main(int argc, char *argv[])
{
    WilqpaintApp *app;
    int status;

    app = wilqpaint_appNew();
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

