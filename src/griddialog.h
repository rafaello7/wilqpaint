#ifndef GRIDDIALOG_H
#define GRIDDIALOG_H


typedef struct GridOptions GridOptions;

GridOptions *grid_optsNew(void);

void grid_showDialog(GridOptions*, GtkWindow *owner,
        void (*onChange)(GtkWindow*));

gdouble grid_getScale(GridOptions*);
gdouble grid_getXOffset(GridOptions*);
gdouble grid_getYOffset(GridOptions*);
gdouble grid_getSnapXValue(GridOptions*, gdouble value, gdouble zoom);
gdouble grid_getSnapYValue(GridOptions*, gdouble value, gdouble zoom);
gboolean grid_isShow(GridOptions*);
void grid_setIsShow(GridOptions*, gboolean);
const char *grid_getSnapId(GridOptions*);
void grid_setSnapById(GridOptions*, const char*);

void grid_optsFree(GridOptions*);

#endif /* GRIDDIALOG_H */
