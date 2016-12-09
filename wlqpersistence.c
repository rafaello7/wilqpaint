#include <gtk/gtk.h>
#include "wlqpersistence.h"
#include <math.h>
#include <string.h>


unsigned wlq_readU32(FILE *fp)
{
    guint32 val;

    fread(&val, 4, 1, fp);
    return GUINT32_FROM_BE(val);
}

void wlq_writeU32(FILE *fp, unsigned val)
{
    guint32 valBE = GUINT32_TO_BE(val);

    fwrite(&valBE, 4, 1, fp);
}

char *wlq_readString(FILE *fp)
{
    int c, len = 0, alloc = 0;
    char *s = NULL;

    while( (c = fgetc(fp)) > 0 ) {
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

void wlq_writeString(FILE *fp, const char *s)
{
    if( s == NULL )
        s = "";
    fwrite(s, strlen(s) + 1, 1, fp);
}

gdouble wlq_readDouble(FILE *fp)
{
    gint64 mantBE;
    gint16 expBE;

    fread(&mantBE, 8, 1, fp);
    fread(&expBE, 2, 1, fp);
    return ldexp(GINT64_FROM_BE(mantBE), GINT16_FROM_BE(expBE) - 62);
}

void wlq_writeDouble(FILE *fp, gdouble val)
{
    int exp;
    double mant;
    gint64 mantBE;
    gint16 expBE;

    /* store 10 bytes for double value */
    mant = frexp(val, &exp);
    mantBE = GINT64_TO_BE(mant * (1LL << 62));
    expBE = GINT16_TO_BE(exp);
    fwrite(&mantBE, 8, 1, fp);
    fwrite(&expBE, 2, 1, fp);
}

void wlq_readRGBA(FILE *fp, GdkRGBA *color)
{
    color->red = wlq_readDouble(fp);
    color->green = wlq_readDouble(fp);
    color->blue = wlq_readDouble(fp);
    color->alpha = wlq_readDouble(fp);
}

void wlq_writeRGBA(FILE *fp, const GdkRGBA *color)
{
    wlq_writeDouble(fp, color->red);
    wlq_writeDouble(fp, color->green);
    wlq_writeDouble(fp, color->blue);
    wlq_writeDouble(fp, color->alpha);
}

