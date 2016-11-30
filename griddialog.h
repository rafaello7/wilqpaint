#ifndef GRIDDIALOG_H
#define GRIDDIALOG_H


void grid_showDialog(GtkWindow *owner, void (*onChange)(void));

gdouble grid_getScale(void);
gdouble grid_getXOffset(void);
gdouble grid_getYOffset(void);
gdouble grid_getSnapXValue(gdouble);
gdouble grid_getSnapYValue(gdouble);


#endif /* GRIDDIALOG_H */
