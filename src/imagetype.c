#include <gtk/gtk.h>
#include "imagetype.h"
#include <string.h>


static struct FileType {
    char *fileTypeId;
    gchar **extensions;
    gboolean isReadable;
    gboolean isWritable;
    GtkFileFilter *typeFilter;
} *gFileTypes;
unsigned gFileTypeCount;
GtkFileFilter *gAllReadableFilter;


static void addFilter(const char *id, const char *desc,
        gchar **extensions, gboolean isReadable, gboolean isWritable)
{
    struct FileType *ft;
    GString *itemDesc;
    int i;
    char patt[20];

    gFileTypes = g_realloc(gFileTypes,
            (gFileTypeCount+1) * sizeof(*gFileTypes));
    ft = gFileTypes + gFileTypeCount++;
    ft->fileTypeId = g_strdup(id);
    ft->extensions = extensions;
    ft->isReadable = isReadable;
    ft->isWritable = isWritable;
    ft->typeFilter = gtk_file_filter_new();
    itemDesc = g_string_new(desc);
    g_string_append(itemDesc, " (");
    for(i = 0; extensions[i]; ++i) {
        snprintf(patt, sizeof(patt), "*.%s", extensions[i]);
        gtk_file_filter_add_pattern(ft->typeFilter, patt);
        if( i ) 
            g_string_append(itemDesc, ", ");
        g_string_append(itemDesc, "*.");
        g_string_append(itemDesc, extensions[i]);
    }
    g_string_append(itemDesc, ")");
    char *descStr = g_string_free(itemDesc, FALSE);
    gtk_file_filter_set_name(ft->typeFilter, descStr);
    g_free(descStr);
}

static void initFileTypes(void)
{
    GSList *pixbufFormats, *item;
    int i, j;
    char patt[20];

    if( gFileTypeCount )
        return;
    addFilter("wilqpaint", "wilqpaint native",
            g_strsplit("wlq", " ", 1), TRUE, TRUE);

    pixbufFormats = gdk_pixbuf_get_formats();
    for(item = pixbufFormats; item != NULL; item = g_slist_next(item)) {
        GdkPixbufFormat *fmt = item->data;
        addFilter(gdk_pixbuf_format_get_name(fmt),
                gdk_pixbuf_format_get_description(fmt),
                gdk_pixbuf_format_get_extensions(fmt), TRUE,
                gdk_pixbuf_format_is_writable(fmt));
    }
    g_slist_free(pixbufFormats);

    addFilter("PDF", "PDF",
            g_strsplit("pdf", " ", 1), FALSE, TRUE);


    gAllReadableFilter = gtk_file_filter_new();
    gtk_file_filter_set_name(gAllReadableFilter, "All supported files");
    for(i = 0; i < gFileTypeCount; ++i) {
        if( gFileTypes[i].isReadable ) {
            for(j = 0; gFileTypes[i].extensions[j]; ++j) {
                snprintf(patt, sizeof(patt), "*.%s",
                        gFileTypes[i].extensions[j]);
                gtk_file_filter_add_pattern(gAllReadableFilter, patt);
            }
        }
    }
}

unsigned imgtype_count(void)
{
    initFileTypes();
    return gFileTypeCount;
}

const char *imgtype_getId(unsigned idx)
{
    initFileTypes();
    return gFileTypes[idx].fileTypeId;
}

gboolean imgtype_isReadable(unsigned idx)
{
    initFileTypes();
    return gFileTypes[idx].isReadable;
}

gboolean imgtype_isWritable(unsigned idx)
{
    initFileTypes();
    return gFileTypes[idx].isWritable;
}

const char *imgtype_getDesc(unsigned idx)
{
    initFileTypes();
    return gtk_file_filter_get_name(gFileTypes[idx].typeFilter);
}

int imgtype_getIdxById(const char *id)
{
    int i;

    initFileTypes();
    for(i = 0; i < gFileTypeCount; ++i) {
        if( !strcmp(id, gFileTypes[i].fileTypeId) )
            return i;
    }
    return -1;
}

int imgtype_getIdxByFileName(const char *fileName)
{
    int i, j;
    const char *ext;

    initFileTypes();
    if( (ext = strrchr(fileName, '.')) != NULL ) {
        ++ext;
        for(i = 0; i < gFileTypeCount; ++i) {
            for(j = 0; gFileTypes[i].extensions[j]; ++j) {
                if( !strcmp(ext, gFileTypes[i].extensions[j]) )
                    return i;
            }
        }
    }
    return -1;
}

GtkFileFilter *imgtype_getFilter(unsigned idx)
{
    initFileTypes();
    return gFileTypes[idx].typeFilter;
}

const char *imgtype_getDefaultExt(unsigned idx)
{
    initFileTypes();
    return gFileTypes[idx].extensions[0];
}

GtkFileFilter *imgtype_getAllReadableFilter(void)
{
    initFileTypes();
    return gAllReadableFilter;
}

gboolean imgtype_isWritableByFileName(const char *fileName)
{
    int idx = imgtype_getIdxByFileName(fileName);
    return imgtype_isWritable(idx);
}

