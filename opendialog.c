#include <gtk/gtk.h>
#include "opendialog.h"
#include "drawimage.h"
#include "imagefile.h"


static void addFilters(GtkFileChooser *chooser)
{
    GSList *formats, *item;
    int i;
    char patt[20];
    GtkFileFilter *filt;

    filt = gtk_file_filter_new();
    gtk_file_filter_set_name(filt, "All supported files");
    gtk_file_filter_add_pixbuf_formats(filt);
    gtk_file_filter_add_pattern(filt, "*.wlq");
    gtk_file_chooser_add_filter(chooser, filt);

    filt = gtk_file_filter_new();
    gtk_file_filter_set_name(filt, "wilqpaint native (*.wlq)");
    gtk_file_filter_add_pattern(filt, "*.wlq");
    gtk_file_chooser_add_filter(chooser, filt);

    formats = gdk_pixbuf_get_formats();
    for(item = formats; item != NULL; item = g_slist_next(item)) {
        GdkPixbufFormat *fmt = item->data;
        gchar *name = gdk_pixbuf_format_get_description(fmt);
        GString *itemDesc = g_string_new(name);
        g_free(name);
        g_string_append(itemDesc, " (");
        gchar **extensions = gdk_pixbuf_format_get_extensions(fmt);
        filt = gtk_file_filter_new();
        for(i = 0; extensions[i]; ++i) {
            snprintf(patt, sizeof(patt), "*.%s", extensions[i]);
            gtk_file_filter_add_pattern(filt, patt);
            if( i ) 
                g_string_append(itemDesc, ", ");
            g_string_append(itemDesc, "*.");
            g_string_append(itemDesc, extensions[i]);
        }
        g_strfreev(extensions);
        g_string_append(itemDesc, ")");
        char *descStr = g_string_free(itemDesc, FALSE);
        gtk_file_filter_set_name(filt, descStr);
        g_free(descStr);
        gtk_file_chooser_add_filter(chooser, filt);
    }
    g_slist_free(formats);
}

struct CallbackParam {
    DrawImage *di;
    GtkLabel *imageDimensions;
    GtkDrawingArea *previewImage;
};

gboolean on_filePreview_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    struct CallbackParam *par = data;
    if( par->di != NULL ) {
        di_draw(par->di, cr, 1.0);
    }
}

static void on_updatePreview(GtkFileChooser *chooser, gpointer user_data)
{
    struct CallbackParam *par = user_data;
    char *fname = gtk_file_chooser_get_preview_filename(chooser);
    gint imgWidth, imgHeight;
    gdouble scaleX = 1.0, scaleY = 1.0;
    char imageDimText[20];
    gchar *err;
    gboolean isNoEntErr;

    if( par->di != NULL ) {
        di_free(par->di);
        par->di = NULL;
    }
    if( fname != NULL ) {
        par->di = imgfile_open(fname, &err, &isNoEntErr);
        g_free(fname);
        if( par->di != NULL ) {
            imgWidth = di_getWidth(par->di);
            imgHeight = di_getHeight(par->di);
            sprintf(imageDimText, "%dx%d", imgWidth, imgHeight);
            gtk_label_set_text(par->imageDimensions, imageDimText);
            if( imgWidth > 320 )
                scaleX = 320.0 / imgWidth;
            if( imgHeight > 480 )
                scaleY = 480.0 / imgHeight;
            if( scaleX < 1.0 || scaleY < 1.0 ) {
                di_scale(par->di, scaleX < scaleY ? scaleX : scaleY);
                imgWidth = di_getWidth(par->di);
                imgHeight = di_getHeight(par->di);
            }
            gtk_widget_set_size_request(GTK_WIDGET(par->previewImage),
                    imgWidth, imgHeight);
            GdkWindow *gdkWin = gtk_widget_get_window(
                    GTK_WIDGET(par->previewImage));
            if( gdkWin )
                gdk_window_invalidate_rect(gdkWin, NULL, FALSE);
        }else{
            g_free(err);
        }
    }
    gtk_file_chooser_set_preview_widget_active(chooser, par->di != NULL);
}

char *showOpenFileDialog(GtkWindow *owner, const char *curFileName)
{
    GtkBuilder *builder;
    GtkFileChooser *chooser;
    struct CallbackParam par;
    char *result = NULL;

    builder = gtk_builder_new_from_resource(
            "/org/rafaello7/wilqpaint/opendialog.ui");
    chooser = GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "openDialog"));
    par.di = NULL;
    par.imageDimensions = GTK_LABEL(gtk_builder_get_object(builder,
                "imageDimensions"));
    par.previewImage = GTK_DRAWING_AREA(gtk_builder_get_object(builder,
                "previewImage"));
    g_object_unref(builder);
    if( curFileName != NULL ) {
        char *dirname = g_path_get_dirname(curFileName);
        gtk_file_chooser_set_current_folder(chooser, dirname);
        g_free(dirname);
    }
    addFilters(chooser);
    g_signal_connect(chooser, "update-preview",
            G_CALLBACK(on_updatePreview), &par);
    g_signal_connect(par.previewImage, "draw",
            G_CALLBACK(on_filePreview_draw), &par);
    gtk_window_set_transient_for(GTK_WINDOW(chooser), owner);
    if( gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT )
        result = gtk_file_chooser_get_filename(chooser);
    gtk_widget_destroy(GTK_WIDGET(chooser));
    if( par.di != NULL )
        di_free(par.di);
    return result;
}

