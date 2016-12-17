#include <gtk/gtk.h>
#include <cairo/cairo-pdf.h>
#include "drawimage.h"
#include "imagefile.h"
#include <string.h>


static const char *getFileFormatByExt(const char *fname)
{
    static char *res;
    GSList *formats, *item;
    const char *ext;
    char *basename;
    int i;

    g_free(res);
    res = NULL;
    basename = g_path_get_basename(fname);
    ext = strrchr(basename, '.');
    if( ext != NULL ) {
        ++ext;
        formats = gdk_pixbuf_get_formats();
        for(item = formats; item != NULL; item = g_slist_next(item)) {
            GdkPixbufFormat *fmt = item->data;
            if( ! gdk_pixbuf_format_is_writable(fmt) )
                continue;
            gchar **extensions = gdk_pixbuf_format_get_extensions(fmt);
            for(i = 0; extensions[i] && res == NULL; ++i) {
                if( ! strcmp(ext, extensions[i]) )
                    res = gdk_pixbuf_format_get_name(fmt);
            }
            g_strfreev(extensions);
        }
        g_slist_free(formats);
    }
    g_free(basename);
    return res;
}

DrawImage *imgfile_open(const char *fileName, gchar **errLoc,
        gboolean *isNoEntErr)
{
    DrawImage *di = NULL;
    int nameLen = strlen(fileName);
    GError *gerr = NULL;

    if( nameLen >= 4 && !strcasecmp(fileName + nameLen - 4, ".wlq") ) {
        di = di_openWLQ(fileName, errLoc, isNoEntErr);
    }else{
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(fileName, &gerr);
        if( pixbuf != NULL ) {
            di = di_new(gdk_pixbuf_get_width(pixbuf),
                    gdk_pixbuf_get_height(pixbuf), pixbuf);
            g_object_unref(pixbuf);
        }else{
            *isNoEntErr =
                    g_error_matches(gerr, G_FILE_ERROR, G_FILE_ERROR_NOENT)
                || g_error_matches(gerr, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);
            *errLoc = g_strdup(gerr->message);
            g_error_free(gerr);
        }
    }
    return di;
}

gboolean imgfile_isFormatWritable(const char *fileName)
{
    GSList *formats, *item;
    const char *ext;
    char *basename;
    int i;
    gboolean res = FALSE;

    basename = g_path_get_basename(fileName);
    ext = strrchr(basename, '.');
    if( ext != NULL ) {
        ++ext;
        if( !strcmp(ext, "wlq") ) {
            res = TRUE;
        }else{
            formats = gdk_pixbuf_get_formats();
            for(item = formats; item != NULL && !res;
                    item = g_slist_next(item))
            {
                GdkPixbufFormat *fmt = item->data;
                if( ! gdk_pixbuf_format_is_writable(fmt) )
                    continue;
                gchar **extensions = gdk_pixbuf_format_get_extensions(fmt);
                for(i = 0; extensions[i] && ! res; ++i) {
                    if( ! strcmp(ext, extensions[i]) )
                        res = TRUE;
                }
                g_strfreev(extensions);
            }
            g_slist_free(formats);
        }
    }
    g_free(basename);
    return res;
}

gboolean imgfile_save(DrawImage *di, const char *fileName, gchar **errLoc)
{
    int nameLen = strlen(fileName);
    GError *gerr = NULL;
    gboolean isOK;
    gint imgWidth, imgHeight;
    const char *fileFmt;

    imgWidth = di_getWidth(di);
    imgHeight = di_getHeight(di);
    di_selectionSetEmpty(di);
    if( nameLen >= 4 && !strcasecmp(fileName + nameLen - 4, ".wlq") ) {
        isOK = di_saveWLQ(di, fileName, errLoc);
    }else if( nameLen >= 4 && !strcasecmp(fileName + nameLen - 4, ".pdf") ) {
        double a4Width = 8.27 * 72.0, a4Height = 11.7 * 72.0;
        cairo_surface_t *paintImage = cairo_pdf_surface_create(fileName,
                imgWidth, imgHeight);
        cairo_status_t status = cairo_surface_status(paintImage);
        if( status == CAIRO_STATUS_SUCCESS ) {
            cairo_t *cr = cairo_create(paintImage);
            di_draw(di, cr, 1.0);
            cairo_destroy(cr);
        }else{
            *errLoc = g_strdup_printf("%s: %s", fileName,
                    cairo_status_to_string(status));
            isOK = FALSE;
        }
        cairo_surface_destroy(paintImage);
    }else{
        fileFmt = getFileFormatByExt(fileName);
        if( fileFmt != NULL ) {
            cairo_surface_t *paintImage = cairo_image_surface_create(
                    CAIRO_FORMAT_ARGB32, imgWidth, imgHeight);
            cairo_t *cr = cairo_create(paintImage);
            di_draw(di, cr, 1.0);
            cairo_destroy(cr);
            GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(paintImage,
                    0, 0, imgWidth, imgHeight);
            cairo_surface_destroy(paintImage);
            isOK = gdk_pixbuf_save(pixbuf, fileName, fileFmt, &gerr, NULL);
            g_object_unref(G_OBJECT(pixbuf));
            if( ! isOK ) {
                *errLoc = g_strdup(gerr->message);
                g_error_free(gerr);
            }
        }else{
            *errLoc = g_strdup_printf("%s: file format is unsupported",
                    fileName);
            isOK = FALSE;
        }
    }
    if( isOK )
        di_markSaved(di);
    return isOK;
}

