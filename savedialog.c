#include <gtk/gtk.h>
#include "savedialog.h"


static GtkFileFilter **getFilters(GtkComboBoxText *fileType,
        const char *curFileName)
{
    GSList *formats, *item;
    int i, filterCount = 0;
    const char *curExt = NULL, *itemId;
    char patt[20];
    GtkFileFilter *filt, **filters;

    if( curFileName != NULL ) {
        curExt = g_strrstr(curFileName, ".");
        if( curExt != NULL )
            ++curExt;
    }
    if( curExt == NULL )
        curExt = "wlq";
    formats = gdk_pixbuf_get_formats();
    filters = g_malloc(sizeof(GtkFileFilter*));
    filt = gtk_file_filter_new();
    gtk_file_filter_set_name(filt, "wilqpaint native (*.wlq)");
    gtk_file_filter_add_pattern(filt, "*.wlq");
    filters[filterCount++] = filt;
    gtk_combo_box_text_append(fileType, "wlq",
            gtk_file_filter_get_name(filt));

    for(item = formats; item != NULL; item = g_slist_next(item)) {
        GdkPixbufFormat *fmt = item->data;
        if( !gdk_pixbuf_format_is_writable(fmt) )
            continue;
        gchar *name = gdk_pixbuf_format_get_description(fmt);
        GString *itemDesc = g_string_new(name);
        g_free(name);
        g_string_append(itemDesc, " (");
        gchar **extensions = gdk_pixbuf_format_get_extensions(fmt);
        filt = gtk_file_filter_new();
        itemId = extensions[0];
        for(i = 0; extensions[i]; ++i) {
            snprintf(patt, sizeof(patt), "*.%s", extensions[i]);
            gtk_file_filter_add_pattern(filt, patt);
            if( i ) 
                g_string_append(itemDesc, ", ");
            g_string_append(itemDesc, "*.");
            g_string_append(itemDesc, extensions[i]);
            if( !g_strcmp0(curExt, extensions[i]) )
                itemId = curExt;
        }
        g_string_append(itemDesc, ")");
        char *descStr = g_string_free(itemDesc, FALSE);
        gtk_file_filter_set_name(filt, descStr);
        gtk_combo_box_text_append(fileType, itemId, descStr);
        g_free(descStr);
        g_strfreev(extensions);
        filters = g_realloc(filters, (filterCount+1) * sizeof(GtkFileFilter*));
        filters[filterCount++] = filt;
    }
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(fileType), curExt);
    g_slist_free(formats);
    filters = g_realloc(filters, (filterCount+1) * sizeof(GtkFileFilter*));
    filters[filterCount] = NULL;
    return filters;
}

static void freeFilters(GtkFileFilter **filters)
{
    int i;

    for(i = 0; filters[i]; ++i)
        g_object_unref(filters[i]);
    g_free(filters);
}

void on_fileFilter_changed(GtkComboBox *combo, gpointer user_data)
{
    GtkFileChooser *chooser;
    char *fname, *fnameNew, *dot;
    GtkFileFilter **filters = user_data;

    chooser = GTK_FILE_CHOOSER(gtk_widget_get_toplevel(GTK_WIDGET(combo)));
    fname = gtk_file_chooser_get_current_name(chooser);
    if( (dot = g_strrstr(fname, ".")) != NULL )
        *dot = '\0';
    gtk_file_chooser_set_filter(chooser, g_object_ref(
                filters[gtk_combo_box_get_active(combo)]));
    fnameNew = g_strdup_printf("%s.%s", fname,
            gtk_combo_box_get_active_id(combo));
    gtk_file_chooser_set_current_name(chooser, fnameNew);
    g_free(fname);
    g_free(fnameNew);
}

gchar *showSaveFileDialog(GtkWindow *owner, const char *curFileName)
{
    GtkBuilder *builder;
    GtkFileChooser *chooser;
    GtkComboBoxText *fileType;
    gchar *result = NULL;
    GtkFileFilter **filters;

    builder = gtk_builder_new_from_resource(
            "/org/rafaello7/wilqpaint/savedialog.ui");
    chooser = GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "saveDialog"));
    fileType = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "fileType"));
    g_object_unref(builder);
    filters = getFilters(fileType, curFileName);
    gtk_file_chooser_set_filter(chooser, g_object_ref(
                filters[gtk_combo_box_get_active(GTK_COMBO_BOX(fileType))]));
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
    if( curFileName != NULL ) {
        gtk_file_chooser_set_filename(chooser, curFileName);
    }else
        gtk_file_chooser_set_current_name(chooser, "unnamed.wlq");
    g_signal_connect(fileType, "changed",
            G_CALLBACK(on_fileFilter_changed), filters);
    gtk_window_set_transient_for(GTK_WINDOW(chooser), owner);
    if( gtk_dialog_run(GTK_DIALOG(chooser)) == 1 )
        result = gtk_file_chooser_get_filename(chooser);
    gtk_widget_destroy(GTK_WIDGET(chooser));
    freeFilters(filters);
    return result;
}

