#include <gtk/gtk.h>
#include "savedialog.h"
#include "drawimage.h"
#include "imagetype.h"
#include "imagefile.h"


void on_fileFilter_changed(GtkComboBox *combo, gpointer user_data)
{
    GtkFileChooser *chooser;
    char *fname, *fnameNew, *dot;
    unsigned idx;

    chooser = GTK_FILE_CHOOSER(gtk_widget_get_toplevel(GTK_WIDGET(combo)));
    fname = gtk_file_chooser_get_current_name(chooser);
    if( (dot = g_strrstr(fname, ".")) != NULL )
        *dot = '\0';
    idx = imgtype_getIdxById(gtk_combo_box_get_active_id(combo));
    gtk_file_chooser_set_filter(chooser,
            g_object_ref(imgtype_getFilter(idx)));
    fnameNew = g_strdup_printf("%s.%s", fname, imgtype_getDefaultExt(idx));
    gtk_file_chooser_set_current_name(chooser, fnameNew);
    g_free(fname);
    g_free(fnameNew);
}

gchar *showSaveFileDialog(GtkWindow *owner, const char *curFileName)
{
    GtkBuilder *builder;
    GtkFileChooser *chooser;
    GtkComboBoxText *fileType;
    gchar *curName, *result = NULL;
    int i;

    builder = gtk_builder_new_from_resource(
            "/org/rafaello7/wilqpaint/savedialog.ui");
    chooser = GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "saveDialog"));
    fileType = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "fileType"));
    g_object_unref(builder);
    if( curFileName != NULL ) {
        char *dirname = g_path_get_dirname(curFileName);
        gtk_file_chooser_set_current_folder(chooser, dirname);
        g_free(dirname);
        curName = g_path_get_basename(curFileName);
    }else
        curName = g_strdup("unnamed.wlq");
    for(i = 0; i < imgtype_count(); ++i) {
        if( imgtype_isWritable(i) )
            gtk_combo_box_text_append(fileType, imgtype_getId(i),
                    imgtype_getDesc(i));
    }
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
    gtk_file_chooser_set_current_name(chooser, curName);
    g_signal_connect(fileType, "changed",
            G_CALLBACK(on_fileFilter_changed), NULL);
    /* the manual set causes also "changed" signal to emit */
    i = imgtype_getIdxByFileName(curName);
    if( i >= 0 && imgtype_isWritable(i) )
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(fileType), imgtype_getId(i));
    else
        gtk_combo_box_set_active(GTK_COMBO_BOX(fileType), 0);
    gtk_window_set_transient_for(GTK_WINDOW(chooser), owner);
    if( gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT )
        result = gtk_file_chooser_get_filename(chooser);
    gtk_widget_destroy(GTK_WIDGET(chooser));
    g_free(curName);
    return result;
}

