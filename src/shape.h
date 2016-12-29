#ifndef SHAPE_H
#define SHAPE_H

#include "wlqpersistence.h"

typedef enum {
    ST_FREEFORM,
    ST_LINE,
    ST_TRIANGLE,
    ST_RECT,
    ST_OVAL,
    ST_TEXT,
    ST_ARROW,
    ST_COUNT
} ShapeType;

enum ShapeCorner {
    SC_NONE,
    SC_LEFT_TOP,
    SC_LEFT_BOTTOM,
    SC_RIGHT_TOP,
    SC_RIGHT_BOTTOM
};

enum ShapeParam {
    SP_STROKECOLOR,
    SP_FILLCOLOR,
    SP_TEXTCOLOR,
    SP_THICKNESS,
    SP_ROUND,
    SP_ANGLE,
    SP_LEFTRIGHT,
    SP_TEXT,        /* sets also font if not set yet */
    SP_FONTNAME
};

typedef struct {
    GdkRGBA strokeColor;
    GdkRGBA fillColor;
    GdkRGBA textColor;
    gdouble thickness;
    gdouble round;
    gdouble angle;
    gboolean isRight;
    const char *text;
    const char *fontName;
} ShapeParams;

typedef struct Shape Shape;

Shape *shape_new(ShapeType, gdouble xRef, gdouble yRef, const ShapeParams*);

void shape_ref(Shape*);
void shape_unref(Shape*);

Shape *shape_copyOf(const Shape*);

/* Replaces the shape pointer with the shape duplicate. The old pointer is
 * dereferenced.
 * Returns the new shape reference.
 */
Shape *shape_replaceDup(Shape**);


/* New shape layout: set right and bottom side of the shape.
 */
void shape_layoutNew(Shape *shape, gdouble xRight, gdouble yBottom,
        gboolean even);


/* Shape layout: move shape sides.
 */
void shape_layout(Shape *shape, const Shape *prev, gdouble x, gdouble y,
        enum ShapeCorner, gboolean even);

void shape_move(Shape *shape, const Shape *prev, gdouble x, gdouble y);

ShapeType shape_getType(const Shape*);
void shape_scale(Shape*, gdouble factor);

void shape_getParams(const Shape*, ShapeParams*);

void shape_setParam(Shape*, enum ShapeParam, const ShapeParams*);

enum ShapeCorner shape_cornerHitTest(const Shape *shape, gdouble x,
        gdouble y, gdouble zoom);

gboolean shape_hitTest(const Shape*, gdouble xBeg, gdouble yBeg,
        gdouble xEnd, gdouble yEnd);

void shape_draw(Shape*, cairo_t*, gdouble zoom, gboolean isSelected,
        gboolean isCurrent);

Shape *shape_readFromFile(WlqInFile*, gchar **errLoc);
gboolean shape_writeToFile(const Shape*, WlqOutFile*, gchar **errLoc);

#endif /* SHAPE_H */
