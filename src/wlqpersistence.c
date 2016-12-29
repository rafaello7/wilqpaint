#include <gtk/gtk.h>
#include "wlqpersistence.h"
#include <math.h>
#include <string.h>


static const char wlqmagic[4] = "WLQ";


struct WlqInFile {
    char *fileName;
    GInputStream *inStrm;
};

struct WlqOutFile {
    GOutputStream *outStrm;
};

static gchar *gerrorToErrStr(GError *gerr)
{
    gchar *errStr = g_strdup(gerr->message);
    g_error_free(gerr);
    return errStr;
}

WlqInFile *wlq_openIn(const char *fileName, gchar **errLoc,
        gboolean *isNoEntErr)
{
    GFile *gf;
    GFileInputStream *inStrm;
    WlqInFile *inFile = NULL;
    char magic[sizeof(wlqmagic)];
    GConverter *conv;
    unsigned version;
    GError *gerr = NULL;

    gf = g_file_new_for_path(fileName);
    inStrm = g_file_read(gf, NULL, &gerr);
    g_object_unref(gf);
    if( inStrm != NULL ) {
        inFile = g_malloc(sizeof(WlqInFile));
        inFile->inStrm = G_INPUT_STREAM(inStrm);
        inFile->fileName = g_strdup(fileName);
        gboolean isValid = FALSE;
        if( wlq_read(inFile, magic, sizeof(magic), errLoc) ) {
            if( memcmp(magic, wlqmagic, sizeof(wlqmagic)) ) {
                *errLoc = g_strdup_printf("%s: file is corrupted", fileName);
            }else if( wlq_readU32(inFile, &version, errLoc) ) {
                if( version == 0 ) {
                    isValid = TRUE;
                }else{
                    *errLoc = g_strdup_printf("%s: file is in newer version "
                           " than supported by this program.", fileName);
                }
            }
        }
        if( isValid ) {
            conv = G_CONVERTER(g_zlib_decompressor_new(
                        G_ZLIB_COMPRESSOR_FORMAT_GZIP));
            inFile->inStrm = G_INPUT_STREAM(g_converter_input_stream_new(
                        inFile->inStrm, conv));
            g_object_unref(inStrm);
            g_object_unref(conv);
        }else{
            wlq_closeIn(inFile);
            inFile = NULL;
        }
        *isNoEntErr = FALSE;
    }else{
        *isNoEntErr = g_error_matches(gerr, G_IO_ERROR, G_IO_ERROR_NOT_FOUND)
            || g_error_matches(gerr, G_FILE_ERROR, G_FILE_ERROR_NOENT);
        *errLoc = gerrorToErrStr(gerr);
    }
    return inFile;
}

WlqOutFile *wlq_openOut(const char *fileName, gchar **errLoc)
{
    GFile *gf;
    GFileOutputStream *outStrm;
    WlqOutFile *outFile = NULL;
    GConverter *conv;
    GError *gerr = NULL;

    gf = g_file_new_for_path(fileName);
    outStrm = g_file_replace(gf, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &gerr);
    g_object_unref(gf);
    if( outStrm != NULL ) {
        outFile = g_malloc(sizeof(WlqOutFile));
        outFile->outStrm = G_OUTPUT_STREAM(outStrm);
        if( wlq_write(outFile, wlqmagic, sizeof(wlqmagic), errLoc) &&
                wlq_writeU32(outFile, 0, errLoc) )    /* version */
        {
            conv = G_CONVERTER(g_zlib_compressor_new(
                        G_ZLIB_COMPRESSOR_FORMAT_GZIP, -1));
            outFile->outStrm = g_converter_output_stream_new(
                    outFile->outStrm, conv);
            g_object_unref(outStrm);
            g_object_unref(conv);
        }else{
            wlq_closeOut(outFile);
            outFile = NULL;
        }
    }else
        *errLoc = gerrorToErrStr(gerr);
    return outFile;
}

const char *wlq_getInFileName(const WlqInFile *inFile)
{
    return inFile->fileName;
}

gboolean wlq_read(WlqInFile *inFile, void *buf, gsize size, gchar **errLoc)
{
    gsize bytesRead;
    GError *gerr = NULL;

    if( g_input_stream_read_all(inFile->inStrm, buf, size, &bytesRead,
                NULL, &gerr) )
    {
        if( bytesRead == size )
            return TRUE;
        *errLoc = g_strdup("unexpected end of file");
    }else
        *errLoc = gerrorToErrStr(gerr);
    return FALSE;
}

gboolean wlq_write(WlqOutFile *outFile, const void *data, gsize size,
        gchar **errLoc)
{
    gsize bytesWritten;
    GError *gerr = NULL;

    gboolean res = g_output_stream_write_all(outFile->outStrm, data, size,
            &bytesWritten, NULL, &gerr);
    if( ! res )
        *errLoc = gerrorToErrStr(gerr);
    return res;
}

gboolean wlq_readU16(WlqInFile *inFile, unsigned *val, gchar **errLoc)
{
    guint16 valBE;
    gboolean res;

    if( res = wlq_read(inFile, &valBE, 2, errLoc) )
        *val = GUINT16_FROM_BE(valBE);
    return res;
}

gboolean wlq_writeU16(WlqOutFile *outFile, unsigned val, gchar **errLoc)
{
    guint16 valBE = GUINT16_TO_BE(val);

    return wlq_write(outFile, &valBE, 2, errLoc);
}

gboolean wlq_readU8(WlqInFile *inFile, unsigned *val, gchar **errLoc)
{
    unsigned char cval;
    gboolean res;

    if( res = wlq_read(inFile, &cval, 1, errLoc) )
        *val = cval;
    return res;
}

gboolean wlq_writeU8(WlqOutFile *outFile, unsigned val, gchar **errLoc)
{
    unsigned char cval = val;

    return wlq_write(outFile, &cval, 1, errLoc);
}

gboolean wlq_readU32(WlqInFile *inFile, unsigned *val, gchar **errLoc)
{
    guint32 valBE;
    gboolean res;

    if( res = wlq_read(inFile, &valBE, 4, errLoc) )
        *val = GUINT32_FROM_BE(valBE);
    return res;
}

gboolean wlq_writeU32(WlqOutFile *outFile, unsigned val, gchar **errLoc)
{
    guint32 valBE = GUINT32_TO_BE(val);

    return wlq_write(outFile, &valBE, 4, errLoc);
}

gboolean wlq_readS32(WlqInFile *inFile, int *val, gchar **errLoc)
{
    gint32 valBE;
    gboolean res;

    if( res = wlq_read(inFile, &valBE, 4, errLoc) )
        *val = GINT32_FROM_BE(valBE);
    return res;
}

gboolean wlq_writeS32(WlqOutFile *outFile, int val, gchar **errLoc)
{
    gint32 valBE = GINT32_TO_BE(val);

    return wlq_write(outFile, &valBE, 4, errLoc);
}

char *wlq_readString(WlqInFile *inFile, gchar **errLoc)
{
    int len = 0, alloc = 0;
    char c, *s = NULL;

    while( wlq_read(inFile, &c, 1, errLoc) ) {
        if( len == alloc ) {
            alloc += 32;
            s = g_realloc(s, alloc);
        }
        s[len++] = c;
        if( c == '\0' )
            return s;
    }
    g_free(s);
    return NULL;
}

gboolean wlq_writeString(WlqOutFile *outFile, const char *s, gchar **errLoc)
{
    if( s == NULL )
        s = "";
    return wlq_write(outFile, s, strlen(s) + 1, errLoc);
}

gboolean wlq_readCoordinate(WlqInFile *inFile, gdouble *val, gchar **errLoc)
{
    int nval;
    gboolean res;

    if( res = wlq_readS32(inFile, &nval, errLoc) )
        *val = ldexp(nval, -8);
    return res;
}

gboolean wlq_writeCoordinate(WlqOutFile *outFile, gdouble val, gchar **errLoc)
{
    return wlq_writeS32(outFile, ldexp(val, 8), errLoc);
}


gboolean wlq_readRGBA(WlqInFile *inFile, GdkRGBA *color, gchar **errLoc)
{
    unsigned red, green, blue, alpha;

    if( ! wlq_readU16(inFile, &red, errLoc)
            || ! wlq_readU16(inFile, &green, errLoc)
            || ! wlq_readU16(inFile, &blue, errLoc)
            || ! wlq_readU16(inFile, &alpha, errLoc) )
        return FALSE;
    color->red   = ldexp(red, -15);
    color->green = ldexp(green, -15);
    color->blue  = ldexp(blue, -15);
    color->alpha = ldexp(alpha, -15);
    return TRUE;
}

gboolean wlq_writeRGBA(WlqOutFile *outFile, const GdkRGBA *color,
        gchar **errLoc)
{
    return wlq_writeU16(outFile, ldexp(color->red, 15), errLoc)
            && wlq_writeU16(outFile, ldexp(color->green, 15), errLoc)
            && wlq_writeU16(outFile, ldexp(color->blue, 15), errLoc)
            && wlq_writeU16(outFile, ldexp(color->alpha, 15), errLoc);
}

void wlq_closeIn(WlqInFile *inFile)
{
    g_input_stream_close(inFile->inStrm, NULL, NULL);
    g_object_unref(inFile->inStrm);
    g_free(inFile->fileName);
    g_free(inFile);
}

void wlq_closeOut(WlqOutFile *outFile)
{
    g_output_stream_close(outFile->outStrm, NULL, NULL);
    g_object_unref(outFile->outStrm);
    g_free(outFile);
}

