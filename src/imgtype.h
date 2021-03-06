#ifndef IMGTYPE_H
#define IMGTYPE_H

unsigned imgtype_count(void);
const char *imgtype_getId(unsigned);
gboolean imgtype_isReadable(unsigned);
gboolean imgtype_isWritable(unsigned);
const char *imgtype_getDesc(unsigned);
int imgtype_getIdxById(const char *id);
int imgtype_getIdxByExt(const char *ext);
int imgtype_getIdxByFileName(const char *fileName);
GtkFileFilter *imgtype_getFilter(unsigned);
const char *imgtype_getDefaultExt(unsigned);

GtkFileFilter *imgtype_getAllReadableFilter(void);

gboolean imgtype_isWritableByFileName(const char *fileName);

#endif /* IMGTYPE_H */
