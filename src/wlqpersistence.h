#ifndef WLQPERSISTENCE_H
#define WLQPERSISTENCE_H


typedef struct WlqInFile WlqInFile;
typedef struct WlqOutFile WlqOutFile;


WlqInFile *wlq_openIn(const char *fileName, gchar **errLoc,
        gboolean *isNoEntErr);
WlqOutFile *wlq_openOut(const char *fileName, gchar **errLoc);

const char *wlq_getInFileName(const WlqInFile*);

gboolean wlq_read(WlqInFile*, void *buf, gsize size, gchar **errLoc);
gboolean wlq_write(WlqOutFile*, const void *data, gsize size, gchar **errLoc);

gboolean wlq_readU16(WlqInFile*, unsigned*, gchar **errLoc);
gboolean wlq_writeU16(WlqOutFile*, unsigned, gchar **errLoc);

gboolean wlq_readU32(WlqInFile*, unsigned*, gchar **errLoc);
gboolean wlq_writeU32(WlqOutFile*, unsigned, gchar **errLoc);

gboolean wlq_readS32(WlqInFile*, int*, gchar **errLoc);
gboolean wlq_writeS32(WlqOutFile*, int, gchar **errLoc);

char *wlq_readString(WlqInFile*, gchar **errLoc);
gboolean wlq_writeString(WlqOutFile*, const char*, gchar **errLoc);

gboolean wlq_readCoordinate(WlqInFile*, gdouble*, gchar **errLoc);
gboolean wlq_writeCoordinate(WlqOutFile*, gdouble, gchar **errLoc);

gboolean wlq_readRGBA(WlqInFile*, GdkRGBA*, gchar **errLoc);
gboolean wlq_writeRGBA(WlqOutFile*, const GdkRGBA*, gchar **errLoc);

void wlq_closeIn(WlqInFile*);
void wlq_closeOut(WlqOutFile*);


#endif /* WLQPERSISTENCE_H */
