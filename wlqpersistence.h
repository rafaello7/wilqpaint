#ifndef WLQPERSISTENCE_H
#define WLQPERSISTENCE_H


typedef struct WlqInFile WlqInFile;
typedef struct WlqOutFile WlqOutFile;


WlqInFile *wlq_openIn(const char *fileName);
WlqOutFile *wlq_openOut(const char *fileName);

gssize wlq_read(WlqInFile*, void *buf, gsize size);
void wlq_write(WlqOutFile*, const void *data, gsize size);

unsigned wlq_readU32(WlqInFile*);
void wlq_writeU32(WlqOutFile*, unsigned);

char *wlq_readString(WlqInFile*);
void wlq_writeString(WlqOutFile*, const char*);

gdouble wlq_readDouble(WlqInFile*);
void wlq_writeDouble(WlqOutFile*, gdouble);

void wlq_readRGBA(WlqInFile*, GdkRGBA*);
void wlq_writeRGBA(WlqOutFile*, const GdkRGBA*);


void wlq_closeIn(WlqInFile*);
void wlq_closeOut(WlqOutFile*);


#endif /* WLQPERSISTENCE_H */
