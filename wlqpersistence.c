#include <gtk/gtk.h>
#include "wlqpersistence.h"
#include <math.h>
#include <string.h>


struct WlqInFile {
    GInputStream *inStrm;
};

struct WlqOutFile {
    GOutputStream *outStrm;
};

WlqInFile *wlq_openIn(const char *fileName)
{
    GFile *gf;
    GFileInputStream *inStrm;
    WlqInFile *inFile = NULL;

    gf = g_file_new_for_path(fileName);
    inStrm = g_file_read(gf, NULL, NULL);
    if( inStrm != NULL ) {
        inFile = g_malloc(sizeof(WlqInFile));
        inFile->inStrm = G_INPUT_STREAM(inStrm);
    }
    g_object_unref(gf);
    return inFile;
}

WlqOutFile *wlq_openOut(const char *fileName)
{
    GFile *gf;
    GFileOutputStream *outStrm;
    WlqOutFile *outFile = NULL;

    gf = g_file_new_for_path(fileName);
    outStrm = g_file_replace(gf, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL);
    if( outStrm != NULL ) {
        outFile = g_malloc(sizeof(WlqOutFile));
        outFile->outStrm = G_OUTPUT_STREAM(outStrm);
    }
    g_object_unref(gf);
    return outFile;
}

gssize wlq_read(WlqInFile *inFile, void *buf, gsize size)
{
    gsize bytesRead;

    return g_input_stream_read_all(inFile->inStrm, buf, size,
            &bytesRead, NULL, NULL) ? bytesRead : -1;
}

void wlq_write(WlqOutFile *outFile, const void *data, gsize size)
{
    gsize bytesWritten;

    g_output_stream_write_all(outFile->outStrm, data, size,
            &bytesWritten, NULL, NULL);
}

unsigned wlq_readU32(WlqInFile *inFile)
{
    guint32 val;

    wlq_read(inFile, &val, 4);
    return GUINT32_FROM_BE(val);
}

void wlq_writeU32(WlqOutFile *outFile, unsigned val)
{
    guint32 valBE = GUINT32_TO_BE(val);

    wlq_write(outFile, &valBE, 4);
}

char *wlq_readString(WlqInFile *inFile)
{
    int len = 0, alloc = 0;
    char c, *s = NULL;

    while( wlq_read(inFile, &c, 1) > 0 && c != '\0' ) {
        if( len == alloc ) {
            alloc += 32;
            s = g_realloc(s, alloc);
        }
        s[len++] = c;
    }
    if( len > 0 ) {
        if( len == alloc )
            s = g_realloc(s, len+1);
        s[len] = '\0';
    }
    return s;
}

void wlq_writeString(WlqOutFile *outFile, const char *s)
{
    if( s == NULL )
        s = "";
    wlq_write(outFile, s, strlen(s) + 1);
}

gdouble wlq_readDouble(WlqInFile *inFile)
{
    gint64 mantBE;
    gint16 expBE;

    wlq_read(inFile, &mantBE, 8);
    wlq_read(inFile, &expBE, 2);
    return ldexp(GINT64_FROM_BE(mantBE), GINT16_FROM_BE(expBE) - 62);
}

void wlq_writeDouble(WlqOutFile *outFile, gdouble val)
{
    int exp;
    double mant;
    gint64 mantBE;
    gint16 expBE;

    /* store 10 bytes for double value */
    mant = frexp(val, &exp);
    mantBE = GINT64_TO_BE(mant * (1LL << 62));
    expBE = GINT16_TO_BE(exp);
    wlq_write(outFile, &mantBE, 8);
    wlq_write(outFile, &expBE, 2);
}

void wlq_readRGBA(WlqInFile *inFile, GdkRGBA *color)
{
    color->red = wlq_readDouble(inFile);
    color->green = wlq_readDouble(inFile);
    color->blue = wlq_readDouble(inFile);
    color->alpha = wlq_readDouble(inFile);
}

void wlq_writeRGBA(WlqOutFile *outFile, const GdkRGBA *color)
{
    wlq_writeDouble(outFile, color->red);
    wlq_writeDouble(outFile, color->green);
    wlq_writeDouble(outFile, color->blue);
    wlq_writeDouble(outFile, color->alpha);
}

void wlq_closeIn(WlqInFile *inFile)
{
    g_input_stream_close(inFile->inStrm, NULL, NULL);
    g_object_unref(inFile->inStrm);
    g_free(inFile);
}

void wlq_closeOut(WlqOutFile *outFile)
{
    g_output_stream_close(outFile->outStrm, NULL, NULL);
    g_object_unref(outFile->outStrm);
    g_free(outFile);
}

