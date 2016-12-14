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
    ST_ARROW
} ShapeType;

enum ShapeSide {
    SS_NONE             = 0x0,
    SS_TOP              = 0x1,     /* upper Y coordinate is changed */
    SS_BOTTOM           = 0x2,     /* lower Y coordinate is changed */
    SS_LEFT             = 0x4,     /* left  X coordinate is changed */
    SS_RIGHT            = 0x8,     /* right X coordinate is changed */
    SS_MID              = 0x10,
    SS_CREATE           = 0x20 /* initial draw (free form shape adds points */
};

enum ShapeParam {
    SP_STROKECOLOR,
    SP_FILLCOLOR,
    SP_TEXTCOLOR,
    SP_THICKNESS,
    SP_ANGLE,
    SP_ROUND,
    SP_TEXT,        /* sets also font if not set yet */
    SP_FONTNAME
};

typedef struct {
    GdkRGBA strokeColor;
    GdkRGBA fillColor;
    GdkRGBA textColor;
    gdouble thickness;
    gdouble angle;
    gdouble round;
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

void shape_moveBeg(Shape*);

/* The x and y coordinates are relative to xRef and yRef.
 */
void shape_moveTo(Shape*, gdouble x, gdouble y, enum ShapeSide);

ShapeType shape_getType(const Shape*);
void shape_scale(Shape*, gdouble factor);

void shape_getParams(const Shape*, ShapeParams*);

void shape_setParam(Shape*, enum ShapeParam, const ShapeParams*);

enum ShapeSide shape_hitTest(const Shape*, gdouble xBeg, gdouble yBeg,
        gdouble xEnd, gdouble yEnd);
void shape_draw(Shape*, cairo_t*, gboolean isCurrent, gboolean isSelected);

Shape *shape_readFromFile(WlqInFile*, gchar **errLoc);
gboolean shape_writeToFile(const Shape*, WlqOutFile*, gchar **errLoc);

#endif /* SHAPE_H */
