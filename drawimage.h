#ifndef DRAWIMAGE_H
#define DRAWIMAGE_H

#include "shape.h"

typedef struct DrawImage DrawImage;

DrawImage *di_new(gint imgWidth, gint imgHeight);
DrawImage *di_open(const char *fileName);

gint di_getWidth(const DrawImage*);
gint di_getHeight(const DrawImage*);
gdouble di_getXRef(const DrawImage*);
gdouble di_getYRef(const DrawImage*);

void di_setBackgroundColor(DrawImage*, const GdkRGBA*);
void di_getBackgroundColor(const DrawImage*, GdkRGBA*);

void di_addShape(DrawImage*, ShapeType, gdouble xRef, gdouble yRef,
        const ShapeParams*);
gboolean di_isCurShapeSet(const DrawImage*);
ShapeType di_getCurShapeType(const DrawImage*);
void di_getCurShapeParams(const DrawImage*, ShapeParams*);

void di_undo(DrawImage*);
void di_redo(DrawImage*);

/* Hits selected shape from point.
 *
 * If the extend is TRUE, the shape is added to current selection.
 * If FALSE, the found element becomes the sole element of selection.
 *
 * The specified point is set as selection reference point.
 */
gboolean di_selectionFromPoint(DrawImage*, gdouble x, gdouble y,
        gboolean isRect, gboolean extend);


void di_selectionFromRect(DrawImage*, gdouble x, gdouble y);


void di_setSelectionParam(DrawImage*, enum ShapeParam, const ShapeParams*);

/* Moves all selected shapes from selection reference point to (x, y).
 * The specified point is set as new reference point of selection.
 */
void di_selectionMoveTo(DrawImage*, gdouble x, gdouble y);


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

void di_draw(const DrawImage*, cairo_t*);

void di_save(DrawImage*, const char *fileName);
gboolean di_isModified(const DrawImage*);

void di_free(DrawImage*);

#endif /* DRAWIMAGE_H */
