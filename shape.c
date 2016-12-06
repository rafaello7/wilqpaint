#include <gtk/gtk.h>
#include "shape.h"
#include "hittest.h"
#include <math.h>


struct Shape {
    ShapeType type;
    gdouble xRef, yRef;
    DrawPoint ptEnd;
    DrawPoint *path;
    int ptCount;
    ShapeParams params;
    int drawnTextWidth;
    int drawnTextHeight;
    int refCount;
};

Shape *shape_new(ShapeType type, gdouble xRef, gdouble yRef,
        const ShapeParams *shapeParams)
{
    Shape *shape = g_malloc(sizeof(Shape));
    shape->type = type;
    shape->xRef = xRef;
    shape->yRef = yRef;
    shape->path = NULL;
    shape->ptCount = 0;
    shape->ptEnd.x = 0;
    shape->ptEnd.y = 0;
    shape->params = *shapeParams;
    if( shapeParams->text != NULL && shapeParams->text[0] &&
            shapeParams->fontName != NULL && shapeParams->fontName[0] )
    {
        shape->params.text = g_strdup(shapeParams->text);
        shape->params.fontName = g_strdup(shapeParams->fontName);
    }else{
        shape->params.text = NULL;
        shape->params.fontName = NULL;
    }
    shape->drawnTextWidth = 0;
    shape->drawnTextHeight = 0;
    shape->refCount = 1;
    return shape;
}

void shape_ref(Shape *shape)
{
    ++shape->refCount;
}

void shape_unref(Shape *shape)
{
    if( --shape->refCount == 0 ) {
        g_free(shape->path);
        g_free((void*)shape->params.text);
        g_free((void*)shape->params.fontName);
    }
}

Shape *shape_copyOf(const Shape *shape)
{
    int i;

    Shape *copy = shape_new(shape->type, shape->xRef, shape->yRef,
            &shape->params);
    if( shape->path != NULL ) {
        copy->path = g_malloc(shape->ptCount * sizeof(*shape->path));
        for(i = 0; i < shape->ptCount; ++i)
            copy->path[i] = shape->path[i];
        copy->ptCount = shape->ptCount;
    }
    copy->ptEnd = shape->ptEnd;
    copy->drawnTextWidth = shape->drawnTextWidth;
    copy->drawnTextHeight = shape->drawnTextHeight;
    return copy;
}

Shape *shape_replaceDup(Shape **pShape)
{
    Shape *shapeOld = *pShape;
    if( shapeOld->refCount > 1 ) {
        *pShape = shape_copyOf(shapeOld);
        shape_unref(shapeOld);
    }
    return *pShape;
}

void shape_addPoint(Shape *shape, gdouble x, gdouble y)
{
    shape->ptEnd.x = x - shape->xRef;
    shape->ptEnd.y = y - shape->yRef;
    if( shape->type == ST_FREEFORM ) {
        shape->path = g_realloc(shape->path,
                ++shape->ptCount * sizeof(DrawPoint));
        shape->path[shape->ptCount-1].x = shape->ptEnd.x;
        shape->path[shape->ptCount-1].y = shape->ptEnd.y;
    }
}

ShapeType shape_getType(const Shape *shape)
{
    return shape->type;
}

gdouble shape_getXRef(const Shape *shape)
{
    return shape->xRef;
}

gdouble shape_getYRef(const Shape *shape)
{
    return shape->yRef;
}

void shape_moveTo(Shape *shape, gdouble xRef, gdouble yRef)
{
    shape->xRef = xRef;
    shape->yRef = yRef;
}

void shape_moveRel(Shape *shape, gdouble x, gdouble y)
{
    shape->xRef += x;
    shape->yRef += y;
}

void shape_scale(Shape *shape, gdouble factor)
{
    int i;
    PangoFontDescription *desc;

    shape->xRef *= factor;
    shape->yRef *= factor;
    shape->ptEnd.x *= factor;
    shape->ptEnd.y *= factor;
    for(i = 0; i < shape->ptCount; ++i) {
        shape->path[i].x *= factor;
        shape->path[i].y *= factor;
    }
    shape->params.thickness *= factor;
    shape->params.round *= factor;
    if( shape->params.fontName != NULL ) {
        desc = pango_font_description_from_string(shape->params.fontName);
        i = pango_font_description_get_size(desc);
        pango_font_description_set_size(desc, MAX(1, round(factor * i)));
        g_free((void*)shape->params.fontName);
        shape->params.fontName = pango_font_description_to_string(desc);
        pango_font_description_free(desc);
    }
}

void shape_getParams(const Shape *shape, ShapeParams *shapeParams)
{
    *shapeParams = shape->params;
}

void shape_setParam(Shape *shape, enum ShapeParam shapeParam,
        const ShapeParams *shapeParams)
{
    switch( shapeParam ) {
    case SP_STROKECOLOR:
        shape->params.strokeColor = shapeParams->strokeColor;
        break;
    case SP_FILLCOLOR:
        shape->params.fillColor = shapeParams->fillColor;
        break;
    case SP_TEXTCOLOR:
        shape->params.textColor = shapeParams->textColor;
        break;
    case SP_THICKNESS:
        shape->params.thickness = shapeParams->thickness;
        break;
    case SP_ANGLE:
        shape->params.angle = shapeParams->angle;
        break;
    case SP_ROUND:
        shape->params.round = shapeParams->round;
        break;
    case SP_TEXT:
        g_free((void*)shape->params.text);
        if( shapeParams->text != NULL && shapeParams->text[0] &&
                (shape->params.fontName != NULL && shape->params.fontName[0] ||
                 shapeParams->fontName != NULL && shapeParams->fontName[0]) )
        {
            shape->params.text = g_strdup(shapeParams->text);
            if( shape->params.fontName == NULL )
                shape->params.fontName = g_strdup(shapeParams->fontName);
        }else{
            shape->params.text = NULL;
        }
        break;
    case SP_FONTNAME:
        if( shapeParams->fontName != NULL && shapeParams->fontName[0] ) {
            g_free((void*)shape->params.fontName);
            shape->params.fontName = g_strdup(shapeParams->fontName);
        }
        break;
    }
}

gboolean shape_hitTest(const Shape *shape, gdouble x, gdouble y,
        gdouble width, gdouble height)
{
    gboolean res = FALSE;

    if( width < 0 ) {
        x += width;
        width = -width;
    }
    if( height < 0 ) {
        y += height;
        height = -height;
    }
    x -= shape->xRef;
    y -= shape->yRef;
    switch( shape->type ) {
    case ST_FREEFORM:
        res = hittest_path(shape->path, shape->ptCount,
                shape->params.thickness, x, y, x+width, y+height);
        break;
    case ST_LINE:
        res = hittest_line(0, 0, shape->ptEnd.x, shape->ptEnd.y,
                shape->params.thickness, x, y, x + width, y + height);
        break;
    case ST_TRIANGLE:
        {
            gdouble c = tan(shape->params.angle * G_PI / 360);
            res = hittest_triangle(0, 0,
                    shape->ptEnd.x + shape->ptEnd.y * c,
                    shape->ptEnd.y - shape->ptEnd.x * c,
                    shape->ptEnd.x - shape->ptEnd.y * c,
                    shape->ptEnd.y + shape->ptEnd.x * c,
                    shape->params.thickness, x, y, x+width, y+height);
        }
        break;
    case ST_RECT:
        res = hittest_rect(0, 0, shape->ptEnd.x, shape->ptEnd.y,
                shape->params.thickness, x, y, x + width, y + height);
        break;
    case ST_OVAL:
        res = hittest_ellipse(0, 0, shape->ptEnd.x, shape->ptEnd.y,
                shape->params.thickness, x, y, x + width, y + height);
        break;
    case ST_TEXT:
        res = hittest_rect(
                shape->ptEnd.x - 0.5 * shape->drawnTextWidth,
                shape->ptEnd.y - 0.5 * shape->drawnTextHeight,
                shape->ptEnd.x + 0.5 * shape->drawnTextWidth,
                shape->ptEnd.y + 0.5 * shape->drawnTextHeight,
                shape->params.thickness, x, y, x + width, y + height);
        break;
    default:
        res = hittest_arrow(0, 0, shape->ptEnd.x, shape->ptEnd.y,
                shape->params.thickness, x, y, x + width, y + height);
        break;
    }
    return res;
}

static void strokeSelection(cairo_t *cr)
{
    gdouble dashes[2];

    cairo_set_line_width(cr, 1);
    dashes[0] = 4;
    dashes[1] = 4;
    cairo_set_dash(cr, dashes, 2, 0);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_stroke_preserve(cr);
    cairo_set_dash(cr, dashes, 2, 4);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_stroke(cr);
    cairo_set_dash(cr, NULL, 0, 0);
}

static void strokeShape(const Shape *shape, cairo_t *cr,
        gboolean isSelected)
{
    gdk_cairo_set_source_rgba (cr, &shape->params.strokeColor);
    cairo_set_line_width(cr, MAX(shape->params.thickness, 1));
    if( isSelected ) {
        cairo_stroke_preserve(cr);
        strokeSelection(cr);
    }else
        cairo_stroke(cr);
}

static void strokeAndFillShape(const Shape *shape, cairo_t *cr,
        gboolean isSelected)
{
    if( shape->params.thickness == 0 || shape->params.strokeColor.alpha == 1) {
        if( shape->params.fillColor.alpha != 0 ) {
            gdk_cairo_set_source_rgba(cr, &shape->params.fillColor);
            cairo_fill_preserve(cr);
        }
        if( shape->params.thickness != 0 ) {
            cairo_set_line_width(cr, shape->params.thickness);
            gdk_cairo_set_source_rgba(cr, &shape->params.strokeColor);
            cairo_stroke_preserve(cr);
        }
    }else{
        cairo_push_group(cr);
        if( shape->params.fillColor.alpha != 0 ) {
            gdk_cairo_set_source_rgba(cr, &shape->params.fillColor);
            cairo_fill_preserve(cr);
        }
        gdk_cairo_set_source_rgba(cr, &shape->params.strokeColor);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_line_width(cr, shape->params.thickness);
        cairo_stroke_preserve(cr);
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        cairo_pop_group_to_source(cr);
        cairo_paint(cr);
    }
    if( isSelected )
        strokeSelection(cr);
    else
        cairo_new_path(cr);
}

static double getAngle(gdouble x, gdouble y)
{
    double res;

    if( y == 0 ) {
        res = x > 0 ? 0 : G_PI;
    }else{
        res = G_PI / 2.0 - atan(x/y);
        if( y < 0 )
            res += G_PI;
    }
    return res;
}

/* Adds to path an arc tangential to two lines: first line is defined by points
 * (xBeg, yBeg), (xEnd, yEnd). Second line is starts at
 * (xEnd, yEnd) and is at the specified angle to the first line.
 */
static void drawArc(cairo_t *cr, gdouble xBeg, gdouble yBeg, gdouble xEnd,
        gdouble yEnd, gdouble angle, gint radius)
{
    double x = xEnd - xBeg, y = yEnd - yBeg;
    gdouble fact = radius / sqrt(x * x + y * y);
    gdouble angleTan = tan( angle * G_PI / 360.0 );
    double line1Angle = getAngle(x, y);

    cairo_arc(cr, xEnd - fact * (x / angleTan + y),
            yEnd - fact * (y / angleTan - x), radius, line1Angle - G_PI/2,
            line1Angle + G_PI/2 - angle * G_PI / 180);
}

static void drawArrow(cairo_t *cr, gdouble shapeXRef, gdouble shapeYRef,
        gdouble shapeXEnd, gdouble shapeYEnd, gdouble thickness, gint angle,
        const GdkRGBA *color, gboolean isSelected)
{
    double xBeg, yBeg, fact;

    if( thickness < 1.0 )
        thickness = 1.0;
    fact = 2.0 + 6.0 / thickness;
    if( shapeXEnd != 0 || shapeYEnd != 0 ) {
        double lineWidth = MAX(thickness, 1);
        double minLen;
        double lineLen = sqrt(shapeXEnd * shapeXEnd + shapeYEnd * shapeYEnd);
        double angleTan = tan(angle * G_PI / 360);
        double xEnd = 0.5 * fact * lineWidth / angleTan * shapeXEnd / lineLen;
        double yEnd = 0.5 * fact * lineWidth / angleTan * shapeYEnd / lineLen;
        cairo_move_to(cr, shapeXRef + shapeXEnd, shapeYRef + shapeYEnd);
        cairo_line_to(cr, shapeXRef + shapeXEnd - xEnd - yEnd * angleTan,
                shapeYRef + shapeYEnd - yEnd + xEnd * angleTan);
        if( angle < 90 ) {
            xBeg = xEnd * 0.5 * (1 + angleTan * angleTan);
            yBeg = yEnd * 0.5 * (1 + angleTan * angleTan);
            minLen = 0.5 * lineWidth * ((0.5 * fact + 0.5) / angleTan
                    + (0.5 * fact - 0.5) * angleTan);
        }else{
            xBeg = xEnd;
            yBeg = yEnd;
            minLen = 0.5 * fact * lineWidth / angleTan;
        }
        if( lineLen > minLen ) {
            cairo_line_to(cr,
                shapeXRef + shapeXEnd - (xEnd + yEnd * angleTan)/fact
                     - xBeg * (1 - 1 / fact),
                shapeYRef + shapeYEnd - (yEnd - xEnd * angleTan)/fact
                     - yBeg * (1 - 1 / fact));
            cairo_line_to(cr, shapeXRef - lineWidth * shapeYEnd / lineLen / 2,
                    shapeYRef + shapeYEnd - shapeYEnd
                    + lineWidth * shapeXEnd / lineLen / 2);
            cairo_line_to(cr, shapeXRef + lineWidth * shapeYEnd / lineLen / 2,
                    shapeYRef + shapeYEnd - shapeYEnd
                    - lineWidth * shapeXEnd / lineLen / 2);
            cairo_line_to(cr,
                shapeXRef + shapeXEnd - (xEnd - yEnd * angleTan)/fact
                     - xBeg * (1 - 1 / fact),
                shapeYRef + shapeYEnd - (yEnd + xEnd * angleTan)/fact
                     - yBeg * (1 - 1 / fact));
        }else if( angle < 90 ) {
            cairo_line_to(cr, shapeXRef + shapeXEnd - xBeg,
                    shapeYRef + shapeYEnd - yBeg);
        }
        cairo_line_to(cr, shapeXRef + shapeXEnd - xEnd + yEnd * angleTan,
                shapeYRef + shapeYEnd - yEnd - xEnd * angleTan);
        cairo_close_path(cr);
        gdk_cairo_set_source_rgba (cr, color);
        cairo_fill(cr);
        if( isSelected ) {
            cairo_move_to(cr, shapeXRef, shapeYRef);
            cairo_line_to(cr, shapeXRef + shapeXEnd, shapeYRef + shapeYEnd);
            strokeSelection(cr);
        }
    }
}

static void drawText(cairo_t *cr, Shape *shape,
        gdouble posFactor, gboolean drawBackground, gboolean isSelected)
{
    PangoLayout *layout;
    PangoFontDescription *desc;
    int width, height;
    gdouble xPaint, yPaint;

    if( shape->params.text == NULL || shape->params.text[0] == '\0' )
        return;
    xPaint = shape->xRef + posFactor * shape->ptEnd.x;
    yPaint = shape->yRef + posFactor * shape->ptEnd.y;
    layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, shape->params.text, -1);
    desc = pango_font_description_from_string(shape->params.fontName);
    pango_layout_set_font_description (layout, desc);
    pango_font_description_free (desc);
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    pango_layout_get_pixel_size(layout, &width, &height);
    if( drawBackground && shape->params.fillColor.alpha != 0.0 || isSelected ) {
        cairo_rectangle(cr, xPaint - 0.5 * (width + shape->params.thickness),
                yPaint - 0.5 * (height + shape->params.thickness),
                width + shape->params.thickness,
                height + shape->params.thickness);
        if( drawBackground && shape->params.fillColor.alpha != 0.0 ) {
            gdk_cairo_set_source_rgba(cr, &shape->params.fillColor);
            cairo_fill_preserve(cr);
        }
        if( isSelected )
            strokeSelection(cr);
        else
            cairo_new_path(cr);
    }
    gdk_cairo_set_source_rgba(cr, &shape->params.textColor);
    cairo_move_to(cr, xPaint - 0.5 * width, yPaint - 0.5 * height);
    pango_cairo_show_layout(cr, layout);
    g_object_unref(layout);
    /* pango_cairo_show_layout does not clear path */
    cairo_new_path(cr);
    shape->drawnTextWidth = width;
    shape->drawnTextHeight = height;
}

void shape_draw(Shape *shape, cairo_t *cr, gboolean isSelected)
{
    cairo_matrix_t matrix;
    double xBeg, yBeg, xEnd, yEnd, angleTan, round, lineLen;

    switch( shape->type ) {
    case ST_FREEFORM:
        if( shape->ptCount > 0 ) {
            cairo_move_to(cr, shape->xRef, shape->yRef);
            for(int j = 0; j < shape->ptCount; ++j)
                cairo_line_to(cr, shape->xRef + shape->path[j].x,
                        shape->yRef + shape->path[j].y);
            strokeShape(shape, cr, isSelected);
        }
        break;
    case ST_LINE:
        if( shape->ptEnd.x != 0 || shape->ptEnd.y != 0 ) {
            cairo_move_to(cr, shape->xRef, shape->yRef);
            cairo_line_to(cr, shape->xRef + shape->ptEnd.x,
                    shape->yRef + shape->ptEnd.y);
            strokeShape(shape, cr, isSelected);
        }
        break;
    case ST_TRIANGLE:
        if( shape->ptEnd.x != 0 || shape->ptEnd.y != 0 ) {
            angleTan = tan(shape->params.angle * G_PI / 360);
            double angleSin = sin(shape->params.angle * G_PI / 360);
            double height = sqrt(shape->ptEnd.x * shape->ptEnd.x
                        + shape->ptEnd.y * shape->ptEnd.y);
            if( shape->params.round == 0 ) {
                cairo_move_to(cr, shape->xRef, shape->yRef);
                cairo_line_to(cr, shape->xRef
                        + shape->ptEnd.x + shape->ptEnd.y * angleTan,
                        shape->yRef + shape->ptEnd.y
                        - shape->ptEnd.x * angleTan);
                cairo_line_to(cr, shape->xRef
                        + shape->ptEnd.x - shape->ptEnd.y * angleTan,
                        shape->yRef + shape->ptEnd.y
                        + shape->ptEnd.x * angleTan);
                cairo_close_path(cr);
            }else if( shape->params.round
                    >= height * (1.0 - 1.0 / (1.0 + angleSin)) )
            {
                double fact = 1.0 / (1.0 + angleSin);
                cairo_arc(cr, shape->xRef + shape->ptEnd.x * fact,
                        shape->yRef + shape->ptEnd.y * fact,
                        height * (1.0 - fact), 0, 2 * G_PI);
            }else{
                double x1 = shape->ptEnd.x + shape->ptEnd.y * angleTan;
                double y1 = shape->ptEnd.y - shape->ptEnd.x * angleTan;
                double x2 = shape->ptEnd.x - shape->ptEnd.y * angleTan;
                double y2 = shape->ptEnd.y + shape->ptEnd.x * angleTan;
                drawArc(cr, shape->xRef + x2, shape->yRef + y2,
                        shape->xRef, shape->yRef, shape->params.angle,
                        shape->params.round);
                drawArc(cr, shape->xRef, shape->yRef, shape->xRef + x1,
                        shape->yRef + y1, 90 - shape->params.angle / 2,
                        shape->params.round);
                drawArc(cr, shape->xRef + x1, shape->yRef + y1,
                        shape->xRef + x2, shape->yRef + y2,
                        90 - shape->params.angle / 2, shape->params.round);
                cairo_close_path(cr);
            }
            strokeAndFillShape(shape, cr, isSelected);
            drawText(cr, shape, 2.0 / 3.0, FALSE, FALSE);
        }
        break;
    case ST_RECT:
        if( shape->ptEnd.x >= 0 ) {
            xBeg = shape->xRef;
            xEnd = xBeg + shape->ptEnd.x;
        }else{
            xEnd = shape->xRef;
            xBeg = xEnd + shape->ptEnd.x;
        }
        if( shape->ptEnd.y >= 0 ) {
            yBeg = shape->yRef;
            yEnd = yBeg + shape->ptEnd.y;
        }else{
            yEnd = shape->yRef;
            yBeg = yEnd + shape->ptEnd.y;
        }
        round = MIN(shape->params.round, MIN(xEnd - xBeg, yEnd - yBeg) / 2);
        cairo_move_to(cr, xBeg, yBeg + round);
        if( round )
            cairo_arc(cr, xBeg + round, yBeg + round, round,
                    G_PI, 1.5 * G_PI);
        cairo_line_to(cr, xEnd - round, yBeg);
        if( round )
            cairo_arc(cr, xEnd - round, yBeg + round, round,
                    1.5 * G_PI, 2 * G_PI);
        cairo_line_to(cr, xEnd, yEnd - round);
        if( round )
            cairo_arc(cr, xEnd - round, yEnd - round,
                    round, 0, 0.5 * G_PI);
        cairo_line_to(cr, xBeg + round, yEnd);
        if( round )
            cairo_arc(cr, xBeg + round, yEnd - round, round,
                    0.5 * G_PI, G_PI);
        cairo_close_path(cr);
        strokeAndFillShape(shape, cr, isSelected);
        drawText(cr, shape, 0.5, FALSE, FALSE);
        break;
    case ST_OVAL:
        if( shape->ptEnd.x != 0 && shape->ptEnd.y != 0 ) {
            matrix.xx = matrix.yy = 1.0;
            matrix.xy = matrix.yx = matrix.x0 = matrix.y0 = 0;
            cairo_save(cr);
            if( ABS(shape->ptEnd.x) < ABS(shape->ptEnd.y) ) {
                double xMid = shape->xRef + shape->ptEnd.x / 2;
                matrix.xx = ABS(1.0 * shape->ptEnd.x / shape->ptEnd.y);
                matrix.x0 = xMid * (1.0 - matrix.xx);
            }else{
                double yMid = shape->yRef + shape->ptEnd.y / 2;
                matrix.yy = ABS(1.0 * shape->ptEnd.y / shape->ptEnd.x);
                matrix.y0 = yMid * (1.0 - matrix.yy);
            }
            cairo_transform(cr, &matrix);
            cairo_arc(cr, shape->xRef + shape->ptEnd.x / 2,
                    shape->yRef + shape->ptEnd.y / 2,
                    MAX(ABS(shape->ptEnd.x), ABS(shape->ptEnd.y)) / 2,
                    0, 2 * G_PI);
            cairo_restore(cr);
            strokeAndFillShape(shape, cr, isSelected);
            drawText(cr, shape, 0.5, FALSE, FALSE);
        }
        break;
    case ST_TEXT:
        drawText(cr, shape, 1.0, TRUE, isSelected);
        break;
    case ST_ARROW:
        drawArrow(cr, shape->xRef, shape->yRef, shape->ptEnd.x, shape->ptEnd.y,
                shape->params.thickness, shape->params.angle,
                &shape->params.strokeColor, isSelected);
        break;
    }
}

