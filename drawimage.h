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
void di_curShapeAddPoint(DrawImage*, gdouble x, gdouble y);
gdouble di_getCurShapeXRef(const DrawImage*);
gdouble di_getCurShapeYRef(const DrawImage*);
ShapeType di_getCurShapeType(const DrawImage*);
void di_getCurShapeParams(const DrawImage*, ShapeParams*);
void di_setCurShapeParams(DrawImage*, const ShapeParams*);
void di_curShapeMoveTo(DrawImage*, gdouble x, gdouble y);
void di_curShapeRemove(DrawImage*);

void di_undo(DrawImage*);
void di_redo(DrawImage*);

gboolean di_curShapeFromPoint(DrawImage*, gdouble x, gdouble y);

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
