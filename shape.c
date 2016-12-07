#include <gtk/gtk.h>
#include "shape.h"
#include "hittest.h"
#include "shapedrawing.h"
#include <math.h>


struct Shape {
    ShapeType type;
    gdouble xRefLeft;
    gdouble xRefRight;
    gdouble yRefTop;
    gdouble yRefBottom;
    gdouble xLeft;
    gdouble xRight;
    gdouble yTop;
    gdouble yBottom;
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
    shape->xRefLeft = shape->xRefRight = xRef;
    shape->yRefTop = shape->yRefBottom = yRef;
    shape->xLeft = shape->xRight = xRef;
    shape->yTop = shape->yBottom = yRef;
    shape->path = NULL;
    shape->ptCount = 0;
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

    Shape *copy = shape_new(shape->type, shape->xRefLeft, shape->yRefTop,
            &shape->params);
    copy->xRefRight = shape->xRefRight;
    copy->yRefBottom = shape->yRefBottom;
    if( shape->path != NULL ) {
        copy->path = g_malloc(shape->ptCount * sizeof(*shape->path));
        for(i = 0; i < shape->ptCount; ++i)
            copy->path[i] = shape->path[i];
        copy->ptCount = shape->ptCount;
    }
    copy->xLeft = shape->xLeft;
    copy->xRight = shape->xRight;
    copy->yTop = shape->yTop;
    copy->yBottom = shape->yBottom;
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

void shape_moveBeg(Shape *shape)
{
    shape->xRefLeft = shape->xLeft;
    shape->xRefRight = shape->xRight;
    shape->yRefTop = shape->yTop;
    shape->yRefBottom = shape->yBottom;
}

void shape_moveTo(Shape *shape, gdouble x, gdouble y,
        enum ShapeSide side)
{
    if( side & (SS_LEFT|SS_MID) )
        shape->xLeft = shape->xRefLeft + x;
    if( side & (SS_TOP|SS_MID) )
        shape->yTop = shape->yRefTop + y;
    if( side & (SS_RIGHT|SS_MID) )
        shape->xRight = shape->xRefRight + x;
    if( side & (SS_BOTTOM|SS_MID) )
        shape->yBottom = shape->yRefBottom + y;
    if( side & SS_CREATE && shape->type == ST_FREEFORM ) {
        shape->path = g_realloc(shape->path,
                ++shape->ptCount * sizeof(DrawPoint));
        shape->path[shape->ptCount-1].x = shape->xRight - shape->xLeft;
        shape->path[shape->ptCount-1].y = shape->yBottom - shape->yTop;
    }
}

ShapeType shape_getType(const Shape *shape)
{
    return shape->type;
}

void shape_scale(Shape *shape, gdouble factor)
{
    int i;
    PangoFontDescription *desc;

    shape->xRefLeft *= factor;
    shape->xRefRight *= factor;
    shape->yRefTop *= factor;
    shape->yRefBottom *= factor;
    shape->xLeft *= factor;
    shape->xRight *= factor;
    shape->yTop *= factor;
    shape->yBottom *= factor;
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

enum ShapeSide shape_hitTest(const Shape *shape, gdouble xBeg, gdouble yBeg,
        gdouble xEnd, gdouble yEnd)
{
    enum ShapeSide res = SS_NONE;
    enum { CORNERDISTMAX = 4 };

    if( xBeg == xEnd && yBeg == yEnd ) {
        switch( shape->type ) {
        case ST_RECT:
        case ST_OVAL:
            if( fabs(xBeg - shape->xRight) < CORNERDISTMAX &&
                    fabs(yEnd - shape->yTop) < CORNERDISTMAX )
                res = SS_RIGHT | SS_TOP;
            else if( fabs(xBeg - shape->xLeft) < CORNERDISTMAX &&
                    fabs(yBeg - shape->yBottom) < CORNERDISTMAX )
                res = SS_LEFT | SS_BOTTOM;
            /* go forth */
        case ST_LINE:
        case ST_ARROW:
        case ST_TRIANGLE:
            if( fabs(xBeg - shape->xRight) < CORNERDISTMAX &&
                    fabs(yBeg - shape->yBottom) < CORNERDISTMAX )
                res = SS_RIGHT | SS_BOTTOM;
            else if( fabs(xBeg - shape->xLeft) < CORNERDISTMAX &&
                    fabs(yBeg - shape->yTop) < CORNERDISTMAX )
                res = SS_LEFT | SS_TOP;
            break;
        default:
            break;
        }
    }
    if( res == SS_NONE ) {
        if( xEnd < xBeg ) {
            gdouble t = xBeg;
            xBeg = xEnd;
            xEnd = t;
        }
        if( yEnd < yBeg ) {
            gdouble t = yBeg;
            yBeg = yEnd;
            yEnd = t;
        }
        switch( shape->type ) {
        case ST_FREEFORM:
            if( hittest_path(shape->path, shape->ptCount,
                    shape->params.thickness,
                    xBeg - shape->xLeft, yBeg - shape->yTop,
                    xEnd - shape->xLeft, yEnd - shape->yTop) )
                res = SS_MID;
            break;
        case ST_LINE:
            if( hittest_line(shape->xLeft, shape->yTop,
                    shape->xRight, shape->yBottom,
                    shape->params.thickness, xBeg, yBeg, xEnd, yEnd) )
                res = SS_MID;
            break;
        case ST_TRIANGLE:
            if( hittest_triangle(shape->xLeft, shape->yTop,
                    shape->xRight, shape->yBottom,
                    shape->params.angle, shape->params.round,
                    shape->params.thickness, xBeg, yBeg, xEnd, yEnd) )
                res = SS_MID;
            break;
        case ST_RECT:
            if( hittest_rect(shape->xLeft, shape->yTop,
                    shape->xRight, shape->yBottom,
                    shape->params.round, shape->params.thickness,
                    xBeg, yBeg, xEnd, yEnd) )
                res = SS_MID;
            break;
        case ST_OVAL:
            if( hittest_ellipse(shape->xLeft, shape->yTop,
                    shape->xRight, shape->yBottom,
                    shape->params.thickness, xBeg, yBeg, xEnd, yEnd) )
                res = SS_MID;
            break;
        case ST_TEXT:
            if( hittest_rect(
                    shape->xRight - 0.5 * shape->drawnTextWidth,
                    shape->yBottom - 0.5 * shape->drawnTextHeight,
                    shape->xRight + 0.5 * shape->drawnTextWidth,
                    shape->yBottom + 0.5 * shape->drawnTextHeight, 0,
                    2 * shape->params.thickness, xBeg, yBeg, xEnd, yEnd) )
                res = SS_MID;
            break;
        default:
            if( hittest_arrow(shape->xLeft, shape->yTop,
                    shape->xRight, shape->yBottom,
                    shape->params.thickness, xBeg, yBeg, xEnd, yEnd) )
                res = SS_MID;
            break;
        }
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

static void strokeResizePt(cairo_t *cr, gdouble x, gdouble y)
{
    cairo_rectangle(cr, x - 3, y - 3, 6, 6);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);
}

static void strokeResizePoints(const Shape *shape, cairo_t *cr,
        gboolean strokeOppositeResize)
{
    if( strokeOppositeResize ) {
        strokeResizePt(cr, shape->xLeft, shape->yBottom);
        strokeResizePt(cr, shape->xRight, shape->yTop);
    }
    strokeResizePt(cr, shape->xLeft, shape->yTop);
    strokeResizePt(cr, shape->xRight, shape->yBottom);
}

static void strokeAndFillShape(const Shape *shape, cairo_t *cr,
        gboolean isSelected, gboolean strokeOppositeResize)
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
    if( isSelected ) {
        strokeSelection(cr);
    }else
        cairo_new_path(cr);
}

static void drawText(cairo_t *cr, Shape *shape,
        gdouble posFactor, gboolean drawBackground, gboolean isSelected)
{
    PangoLayout *layout = NULL;
    PangoFontDescription *desc;
    int width, height;
    gdouble xPaint, yPaint;
    const char *text = shape->params.text;

    if( ! drawBackground && (text == NULL || text[0] == '\0') )
        return;
    xPaint = shape->xLeft + posFactor * (shape->xRight - shape->xLeft);
    yPaint = shape->yTop + posFactor * (shape->yBottom - shape->yTop);
    if( shape->params.fontName ) {
        layout = pango_cairo_create_layout(cr);
        pango_layout_set_text(layout, text ? text : "", -1);
        desc = pango_font_description_from_string(shape->params.fontName);
        pango_layout_set_font_description (layout, desc);
        pango_font_description_free (desc);
        pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
        pango_layout_get_pixel_size(layout, &width, &height);
    }else{
        width = 1;
        height = 8;
    }
    if( drawBackground && shape->params.fillColor.alpha != 0.0 || isSelected ) {
        cairo_rectangle(cr, xPaint - 0.5 * width - shape->params.thickness,
                yPaint - 0.5 * height - shape->params.thickness,
                width + 2.0 * shape->params.thickness,
                height + 2.0 * shape->params.thickness);
        if( drawBackground && shape->params.fillColor.alpha != 0.0 ) {
            gdk_cairo_set_source_rgba(cr, &shape->params.fillColor);
            cairo_fill_preserve(cr);
        }
        if( isSelected )
            strokeSelection(cr);
        else
            cairo_new_path(cr);
    }
    if( layout ) {
        gdk_cairo_set_source_rgba(cr, &shape->params.textColor);
        cairo_move_to(cr, xPaint - 0.5 * width, yPaint - 0.5 * height);
        pango_cairo_show_layout(cr, layout);
        g_object_unref(layout);
        /* pango_cairo_show_layout does not clear path */
        cairo_new_path(cr);
    }
    shape->drawnTextWidth = width;
    shape->drawnTextHeight = height;
}

void shape_draw(Shape *shape, cairo_t *cr, gboolean isCurrent,
        gboolean isSelected)
{
    switch( shape->type ) {
    case ST_FREEFORM:
        if( shape->ptCount > 0 ) {
            cairo_move_to(cr, shape->xLeft, shape->yTop);
            for(int j = 0; j < shape->ptCount; ++j)
                cairo_line_to(cr, shape->xLeft + shape->path[j].x,
                        shape->yTop + shape->path[j].y);
        }else{
            sd_pathPoint(cr, shape->xLeft, shape->yTop);
        }
        strokeShape(shape, cr, isSelected);
        break;
    case ST_LINE:
        if( shape->xRight != shape->xLeft || shape->yBottom != shape->yTop ) {
            sd_pathLine(cr, shape->xLeft, shape->yTop, shape->xRight,
                    shape->yBottom);
        }else{
            sd_pathPoint(cr, shape->xLeft, shape->yTop);
        }
        strokeShape(shape, cr, isSelected);
        if( isCurrent && isSelected )
            strokeResizePoints(shape, cr, FALSE);
        break;
    case ST_TRIANGLE:
        if( shape->xRight != shape->xLeft || shape->yBottom != shape->yTop ) {
            sd_pathTriangle(cr, shape->xLeft, shape->yTop,
                    shape->xRight, shape->yBottom,
                    shape->params.angle, shape->params.round);
        }else{
            sd_pathPoint(cr, shape->xLeft, shape->yTop);
        }
        strokeAndFillShape(shape, cr, isSelected, FALSE);
        if( isCurrent && isSelected )
            strokeResizePoints(shape, cr, FALSE);
        drawText(cr, shape, 2.0 / 3.0, FALSE, FALSE);
        break;
    case ST_RECT:
        if( shape->xRight != shape->xLeft || shape->yBottom != shape->yTop ) {
            sd_pathRect(cr, shape->xLeft, shape->yTop,
                    shape->xRight, shape->yBottom, shape->params.round);
        }else{
            sd_pathPoint(cr, shape->xLeft, shape->yTop);
        }
        strokeAndFillShape(shape, cr, isSelected, TRUE);
        if( isCurrent && isSelected )
            strokeResizePoints(shape, cr, TRUE);
        drawText(cr, shape, 0.5, FALSE, FALSE);
        break;
    case ST_OVAL:
        if( shape->xRight != shape->xLeft || shape->yBottom != shape->yTop ) {
            sd_pathOval(cr, shape->xLeft, shape->yTop,
                    shape->xRight, shape->yBottom);
        }else{
            sd_pathPoint(cr, shape->xLeft, shape->yTop);
        }
        strokeAndFillShape(shape, cr, isSelected, TRUE);
        if( isCurrent && isSelected )
            strokeResizePoints(shape, cr, TRUE);
        drawText(cr, shape, 0.5, FALSE, FALSE);
        break;
    case ST_TEXT:
        drawText(cr, shape, 1.0, TRUE, isSelected);
        break;
    case ST_ARROW:
        if( shape->xRight != shape->xLeft || shape->yBottom != shape->yTop ) {
            sd_pathArrow(cr, shape->xLeft, shape->yTop,
                    shape->xRight, shape->yBottom,
                    shape->params.thickness, shape->params.angle);
        }else{
            sd_pathPoint(cr, shape->xLeft, shape->yTop);
        }
        gdk_cairo_set_source_rgba (cr, &shape->params.strokeColor);
        cairo_fill(cr);
        if( isSelected ) {
            cairo_move_to(cr, shape->xLeft, shape->yTop);
            cairo_line_to(cr, shape->xRight, shape->yBottom);
            strokeSelection(cr);
            if( isCurrent )
                strokeResizePoints(shape, cr, FALSE);
        }
        break;
    }
}

