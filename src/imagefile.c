#include <gtk/gtk.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-svg.h>
#include "drawimage.h"
#include "imgtype.h"
#include "imagefile.h"
#include <string.h>


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

gboolean imgfile_save(DrawImage *di, const char *fileName, gchar **errLoc)
{
    int nameLen = strlen(fileName);
    GError *gerr = NULL;
    gboolean isOK;
    gint imgWidth, imgHeight, typeIdx;

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
        isOK = status == CAIRO_STATUS_SUCCESS;
        if( isOK ) {
            cairo_t *cr = cairo_create(paintImage);
            di_draw(di, cr, 1.0);
            cairo_destroy(cr);
        }else{
            *errLoc = g_strdup_printf("%s: %s", fileName,
                    cairo_status_to_string(status));
        }
        cairo_surface_destroy(paintImage);
    }else if( nameLen >= 4 && !strcasecmp(fileName + nameLen - 4, ".svg") ) {
        cairo_surface_t *paintImage = cairo_svg_surface_create(fileName,
                imgWidth, imgHeight);
        cairo_status_t status = cairo_surface_status(paintImage);
        isOK = status == CAIRO_STATUS_SUCCESS;
        if( isOK ) {
            cairo_t *cr = cairo_create(paintImage);
            di_draw(di, cr, 1.0);
            cairo_destroy(cr);
        }else{
            *errLoc = g_strdup_printf("%s: %s", fileName,
                    cairo_status_to_string(status));
        }
        cairo_surface_destroy(paintImage);
    }else{
        typeIdx = imgtype_getIdxByFileName(fileName);
        if( typeIdx >= 0 && imgtype_isWritable(typeIdx) ) {
            cairo_surface_t *paintImage = cairo_image_surface_create(
                    CAIRO_FORMAT_ARGB32, imgWidth, imgHeight);
            cairo_t *cr = cairo_create(paintImage);
            di_draw(di, cr, 1.0);
            cairo_destroy(cr);
            GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(paintImage,
                    0, 0, imgWidth, imgHeight);
            cairo_surface_destroy(paintImage);
            isOK = gdk_pixbuf_save(pixbuf, fileName,
                    imgtype_getId(typeIdx), &gerr, NULL);
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

