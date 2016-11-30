#ifndef SIZEDIALOG_H
#define SIZEDIALOG_H

gboolean showSizeDialog(GtkWindow *owner, const char *title,
        gdouble *width, gdouble *height, gboolean keepRatio);

#endif /* SIZEDIALOG_H */
