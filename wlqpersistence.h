#ifndef WLQPERSISTENCE_H
#define WLQPERSISTENCE_H

unsigned wlq_readU32(FILE*);
void wlq_writeU32(FILE*, unsigned);

char *wlq_readString(FILE *fp);
void wlq_writeString(FILE*, const char*);

gdouble wlq_readDouble(FILE*);
void wlq_writeDouble(FILE*, gdouble);

void wlq_readRGBA(FILE*, GdkRGBA*);
void wlq_writeRGBA(FILE*, const GdkRGBA*);


#endif /* WLQPERSISTENCE_H */
