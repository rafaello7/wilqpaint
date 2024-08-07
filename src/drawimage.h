#ifndef DRAWIMAGE_H
#define DRAWIMAGE_H

#include "shape.h"

typedef struct DrawImage DrawImage;

DrawImage *di_new(gint imgWidth, gint imgHeight, const GdkPixbuf *baseImage);

DrawImage *di_openWLQ(const char *fileName, gchar **errLoc,
        gboolean *isNoEntErr);

gint di_getWidth(const DrawImage*);
gint di_getHeight(const DrawImage*);
gdouble di_getXRef(const DrawImage*);
gdouble di_getYRef(const DrawImage*);
gboolean di_hasBaseImage(const DrawImage*);

void di_getBackgroundColor(const DrawImage*, GdkRGBA*);
void di_setBackgroundColor(DrawImage*, const GdkRGBA*);

void di_addShape(DrawImage*, ShapeType, gdouble xRef, gdouble yRef,
        const ShapeParams*, gboolean addBottom);
gboolean di_isSelectionEmpty(const DrawImage*);
ShapeType di_getCurShapeType(const DrawImage*);
void di_getCurShapeParams(const DrawImage*, ShapeParams*);
void di_curShapeRaise(DrawImage*);
void di_curShapeSink(DrawImage*);

void di_undo(DrawImage*);
void di_redo(DrawImage*);

/* Hits selected shape from point.
 */
gboolean di_curShapeFromPoint(DrawImage*, gdouble x, gdouble y,
        gdouble zoom, gboolean extendSel);


/* Begins a new rect selection
 */
void di_selectionFromPoint(DrawImage*, gdouble x, gdouble y, gboolean extend);


void di_selectionFromRect(DrawImage*, gdouble x, gdouble y);


void di_setSelectionParam(DrawImage*, enum ShapeParam, const ShapeParams*);


/* Move all selected shapes from selection reference point to (x, y).
 */
void di_selectionDragTo(DrawImage*, gdouble x, gdouble y, gboolean even);


/* Deletes all selected shapes from image
 */
gboolean di_selectionDelete(DrawImage*);


gboolean di_selectionSetEmpty(DrawImage*);


void di_scale(DrawImage*, gdouble factor);

/* Set new xRef and yRef value of image.
 */
void di_moveTo(DrawImage*, gdouble imgXRef, gdouble imgYRef);

/* Set new size of image.
 * The image contents is moved according to factors whose should be
 * in range [0, 1].
 */
void di_setSize(DrawImage*, gint imgWidth, gint imgHeight,
        gdouble translateXfactor, gdouble translateYfactor);

void di_draw(const DrawImage*, cairo_t*, gdouble zoom);
GdkPixbuf *di_toPixbuf(const DrawImage*);

gboolean di_saveWLQ(DrawImage*, const char *fileName, gchar **errLoc);
void di_markSaved(DrawImage*);
gboolean di_isModified(const DrawImage*);

void di_thresholdPreview(DrawImage*, gdouble level);
void di_thresholdFinish(DrawImage*, gboolean commit);

void di_rotate180(DrawImage*);

void di_free(DrawImage*);

void di_dump(DrawImage*);

#endif /* DRAWIMAGE_H */
