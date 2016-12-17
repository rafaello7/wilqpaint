#ifndef IMAGEFILE_H
#define IMAGEFILE_H

DrawImage *imgfile_open(const char *fileName, gchar **errLoc,
        gboolean *isNoEntErr);
gboolean imgfile_save(DrawImage*, const char *fileName, gchar **errLoc);


#endif /* IMAGEFILE_H */
