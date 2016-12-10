#ifndef GRIDDIALOG_H
#define GRIDDIALOG_H


void grid_showDialog(GtkWindow *owner, void (*onChange)(GtkWidget*));

gdouble grid_getScale(void);
gdouble grid_getXOffset(void);
gdouble grid_getYOffset(void);
gdouble grid_getSnapXValue(gdouble);
gdouble grid_getSnapYValue(gdouble);
gboolean grid_isShow(void);
void grid_setIsShow(gboolean);
gboolean grid_isSnapTo(void);
void grid_setIsSnapTo(gboolean);


#endif /* GRIDDIALOG_H */
