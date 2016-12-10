#ifndef GRIDDIALOG_H
#define GRIDDIALOG_H


typedef struct GridOptions GridOptions;

GridOptions *grid_optsNew(void);

void grid_showDialog(GridOptions*, GtkWindow *owner,
        void (*onChange)(GtkWindow*));

gdouble grid_getScale(GridOptions*);
gdouble grid_getXOffset(GridOptions*);
gdouble grid_getYOffset(GridOptions*);
gdouble grid_getSnapXValue(GridOptions*, gdouble);
gdouble grid_getSnapYValue(GridOptions*, gdouble);
gboolean grid_isShow(GridOptions*);
void grid_setIsShow(GridOptions*, gboolean);
gboolean grid_isSnapTo(GridOptions*);
void grid_setIsSnapTo(GridOptions*, gboolean);

void grid_optsFree(GridOptions*);

#endif /* GRIDDIALOG_H */
