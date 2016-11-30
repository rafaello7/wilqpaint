#ifndef SHAPE_H
#define SHAPE_H


typedef enum {
    ST_FREEFORM,
    ST_LINE,
    ST_TRIANGLE,
    ST_RECT,
    ST_OVAL,
    ST_TEXT,
    ST_ARROW
} ShapeType;

typedef struct {
    GdkRGBA strokeColor;
    GdkRGBA fillColor;
    GdkRGBA textColor;
    gdouble thickness;
    int angle;
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

void shape_addPoint(Shape*, gdouble x, gdouble y);

ShapeType shape_getType(const Shape*);
gdouble shape_getXRef(const Shape*);
gdouble shape_getYRef(const Shape*);
void shape_moveTo(Shape*, gdouble xRef, gdouble yRef);
void shape_scale(Shape*, gdouble factor);

void shape_getParams(const Shape*, ShapeParams*);

void shape_setParams(Shape*, const ShapeParams*);

gboolean shape_hitTest(const Shape*, gdouble x, gdouble y);
void shape_draw(const Shape*, cairo_t*);


#endif /* SHAPE_H */
