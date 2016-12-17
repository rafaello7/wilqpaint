#ifndef THRESHOLDDIALOG_H
#define THRESHOLDDIALOG_H

gboolean showThresholdDialog(GtkWindow *owner, gdouble initialValue,
        void (*onChange)(GtkWindow *owner, gdouble));

#endif /* THRESHOLDDIALOG_H */
