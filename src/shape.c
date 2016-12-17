#include <gtk/gtk.h>
#include "shape.h"
#include "hittest.h"
#include "shapedrawing.h"
#include <math.h>
#include <string.h>


struct Shape {
    ShapeType type;
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

    Shape *copy = shape_new(shape->type, shape->xLeft, shape->yTop,
            &shape->params);
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

void shape_layoutNew(Shape *shape, gdouble xRight, gdouble yBottom,
        gboolean even)
{
    if( even ) {
        switch( shape->type ) {
        case ST_LINE:
        case ST_TRIANGLE:
        case ST_ARROW:
            if( fabs(xRight - shape->xLeft) > fabs(yBottom - shape->yTop) ) {
                yBottom = shape->yTop;
            }else{
                xRight = shape->xLeft;
            }
            break;
        case ST_RECT:
        case ST_OVAL:
            if( fabs(xRight - shape->xLeft) > fabs(yBottom - shape->yTop) ) {
                yBottom = shape->yTop
                    + copysign(xRight - shape->xLeft, yBottom - shape->yTop);
            }else{
                xRight = shape->xLeft
                    + copysign(yBottom - shape->yTop, xRight - shape->xLeft);
            }
            break;
        }
    }
    shape->xRight = xRight;
    shape->yBottom = yBottom;
    if( shape->type == ST_FREEFORM ) {
        shape->path = g_realloc(shape->path,
                ++shape->ptCount * sizeof(DrawPoint));
        shape->path[shape->ptCount-1].x = xRight - shape->xLeft;
        shape->path[shape->ptCount-1].y = yBottom - shape->yTop;
    }
}

void shape_layout(Shape *shape, const Shape *prev, gdouble x, gdouble y,
        enum ShapeCorner corner, gboolean even)
{
    gdouble shapeWidth, shapeHeight;

    if( corner == SC_LEFT_TOP || corner == SC_LEFT_BOTTOM )
        shape->xLeft = prev->xLeft + x;
    if( corner == SC_LEFT_TOP || corner == SC_RIGHT_TOP )
        shape->yTop = prev->yTop + y;
    if( corner == SC_RIGHT_TOP || corner == SC_RIGHT_BOTTOM )
        shape->xRight = prev->xRight + x;
    if( corner == SC_LEFT_BOTTOM || corner == SC_RIGHT_BOTTOM )
        shape->yBottom = prev->yBottom + y;

    shapeWidth = shape->xRight - shape->xLeft;
    shapeHeight = shape->yBottom - shape->yTop;
    if( even ) {
        switch( shape->type ) {
        case ST_LINE:
        case ST_TRIANGLE:
        case ST_ARROW:
            if( fabs(shapeWidth) >= fabs(shapeHeight) ) {
                if( corner == SC_LEFT_TOP || corner == SC_RIGHT_TOP )
                    shape->yTop = shape->yBottom;
                else
                    shape->yBottom = shape->yTop;
            }else{
                if( corner == SC_LEFT_TOP || corner == SC_LEFT_BOTTOM )
                    shape->xLeft = shape->xRight;
                else
                    shape->xRight = shape->xLeft;
            }
            break;
        case ST_RECT:
        case ST_OVAL:
            if( fabs(shapeWidth) >= fabs(shapeHeight) ) {
                if( corner == SC_LEFT_TOP || corner == SC_RIGHT_TOP )
                    shape->yTop = shape->yBottom
                        - copysign(shapeWidth, shapeHeight);
                else
                    shape->yBottom = shape->yTop
                        + copysign(shapeWidth, shapeHeight);
            }else{
                if( corner == SC_LEFT_TOP || corner == SC_LEFT_BOTTOM )
                    shape->xLeft = shape->xRight
                        - copysign(shapeHeight, shapeWidth);
                else
                    shape->xRight = shape->xLeft
                        + copysign(shapeHeight, shapeWidth);
            }
            break;
        }
    }
}

void shape_move(Shape *shape, const Shape *prev, gdouble x, gdouble y)
{
    shape->xLeft = prev->xLeft + x;
    shape->yTop = prev->yTop + y;
    shape->xRight = prev->xRight + x;
    shape->yBottom = prev->yBottom + y;
}

ShapeType shape_getType(const Shape *shape)
{
    return shape->type;
}

void shape_scale(Shape *shape, gdouble factor)
{
    int i;
    PangoFontDescription *desc;

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

enum ShapeCorner shape_cornerHitTest(const Shape *shape, gdouble x,
        gdouble y, gdouble zoom)
{
    enum ShapeCorner res = SC_NONE;
    gdouble cornerDistMax = 4.0 / zoom;

    switch( shape->type ) {
    case ST_RECT:
    case ST_OVAL:
        if( fabs(x - shape->xRight) < cornerDistMax &&
                fabs(y - shape->yTop) < cornerDistMax )
            res = SC_RIGHT_TOP;
        else if( fabs(x - shape->xLeft) < cornerDistMax &&
                fabs(y - shape->yBottom) < cornerDistMax )
            res = SC_LEFT_BOTTOM;
        /* go forth */
    case ST_LINE:
    case ST_ARROW:
    case ST_TRIANGLE:
        if( fabs(x - shape->xRight) < cornerDistMax &&
                fabs(y - shape->yBottom) < cornerDistMax )
            res = SC_RIGHT_BOTTOM;
        else if( fabs(x - shape->xLeft) < cornerDistMax &&
                fabs(y - shape->yTop) < cornerDistMax )
            res = SC_LEFT_TOP;
        break;
    default:
        break;
    }
    return res;
}

gboolean shape_hitTest(const Shape *shape, gdouble xBeg, gdouble yBeg,
        gdouble xEnd, gdouble yEnd)
{
    gboolean res = FALSE;

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
        res = hittest_path(shape->path, shape->ptCount,
                shape->params.thickness,
                xBeg - shape->xLeft, yBeg - shape->yTop,
                xEnd - shape->xLeft, yEnd - shape->yTop);
        break;
    case ST_LINE:
        res = hittest_line(shape->xLeft, shape->yTop,
                shape->xRight, shape->yBottom,
                shape->params.thickness, xBeg, yBeg, xEnd, yEnd);
        break;
    case ST_TRIANGLE:
        res = hittest_triangle(shape->xLeft, shape->yTop,
                shape->xRight, shape->yBottom,
                shape->params.angle, shape->params.round,
                shape->params.thickness, xBeg, yBeg, xEnd, yEnd);
        break;
    case ST_RECT:
        res = hittest_rect(shape->xLeft, shape->yTop,
                shape->xRight, shape->yBottom,
                shape->params.round, shape->params.thickness,
                xBeg, yBeg, xEnd, yEnd);
        break;
    case ST_OVAL:
        res = hittest_ellipse(shape->xLeft, shape->yTop,
                shape->xRight, shape->yBottom,
                shape->params.thickness, xBeg, yBeg, xEnd, yEnd);
        break;
    case ST_TEXT:
        res = hittest_rect(
                shape->xRight - 0.5 * shape->drawnTextWidth,
                shape->yBottom - 0.5 * shape->drawnTextHeight,
                shape->xRight + 0.5 * shape->drawnTextWidth,
                shape->yBottom + 0.5 * shape->drawnTextHeight, 0,
                2 * shape->params.thickness, xBeg, yBeg, xEnd, yEnd);
        break;
    default:
        res = hittest_arrow(shape->xLeft, shape->yTop,
                shape->xRight, shape->yBottom,
                shape->params.thickness, xBeg, yBeg, xEnd, yEnd);
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

static void drawTextOnShape(cairo_t *cr, gdouble zoom, Shape *shape,
        gdouble posFactor)
{
    PangoLayout *layout;
    PangoFontDescription *desc;
    int width, height;
    gdouble xPaint, yPaint;

    if( shape->params.text == NULL || shape->params.text[0] == '\0' )
        return;
    xPaint = zoom * (shape->xLeft + posFactor * (shape->xRight - shape->xLeft));
    yPaint = zoom * (shape->yTop + posFactor * (shape->yBottom - shape->yTop));
    layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, shape->params.text, -1);
    desc = pango_font_description_from_string(shape->params.fontName);
    if( zoom != 1.0 ) {
        pango_font_description_set_size(desc,
                pango_font_description_get_size(desc) * zoom);
    }
    pango_layout_set_font_description (layout, desc);
    pango_font_description_free (desc);
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    pango_layout_get_pixel_size(layout, &width, &height);
    cairo_clip_preserve(cr);
    gdk_cairo_set_source_rgba(cr, &shape->params.textColor);
    cairo_move_to(cr, xPaint - 0.5 * width, yPaint - 0.5 * height);
    pango_cairo_show_layout(cr, layout);
    cairo_reset_clip(cr);
    g_object_unref(layout);
    shape->drawnTextWidth = width / zoom;
    shape->drawnTextHeight = height / zoom;
}

static void strokeShape(const Shape *shape, cairo_t *cr,
        gdouble zoom, gboolean isSelected)
{
    gdk_cairo_set_source_rgba (cr, &shape->params.strokeColor);
    cairo_set_line_width(cr, zoom * MAX(shape->params.thickness, 1));
    if( isSelected ) {
        cairo_stroke_preserve(cr);
        strokeSelection(cr);
    }else
        cairo_stroke(cr);
}

static void strokeResizePt(cairo_t *cr, gdouble zoom, gdouble x, gdouble y)
{
    cairo_rectangle(cr, zoom * x - 3, zoom * y - 3, 6, 6);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);
}

static void strokeResizePoints(const Shape *shape, cairo_t *cr,
        gdouble zoom, gboolean strokeOppositeResize)
{
    if( strokeOppositeResize ) {
        strokeResizePt(cr, zoom, shape->xLeft, shape->yBottom);
        strokeResizePt(cr, zoom, shape->xRight, shape->yTop);
    }
    strokeResizePt(cr, zoom, shape->xLeft, shape->yTop);
    strokeResizePt(cr, zoom, shape->xRight, shape->yBottom);
}

static void strokeAndFillShape(Shape *shape, cairo_t *cr, gdouble zoom,
        gboolean isSelected, gboolean strokeOppositeResize, gdouble posFactor)
{
    if( shape->params.thickness == 0 || shape->params.strokeColor.alpha == 1) {
        if( shape->params.fillColor.alpha != 0 ) {
            gdk_cairo_set_source_rgba(cr, &shape->params.fillColor);
            cairo_fill_preserve(cr);
        }
        drawTextOnShape(cr, zoom, shape, posFactor);
        if( shape->params.thickness != 0 ) {
            cairo_set_line_width(cr, zoom * shape->params.thickness);
            gdk_cairo_set_source_rgba(cr, &shape->params.strokeColor);
            cairo_stroke_preserve(cr);
        }
    }else{
        cairo_push_group(cr);
        if( shape->params.fillColor.alpha != 0 ) {
            gdk_cairo_set_source_rgba(cr, &shape->params.fillColor);
            cairo_fill_preserve(cr);
        }
        drawTextOnShape(cr, zoom, shape, posFactor);
        gdk_cairo_set_source_rgba(cr, &shape->params.strokeColor);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_line_width(cr, zoom * shape->params.thickness);
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

static void drawText(cairo_t *cr, gdouble zoom, Shape *shape,
        gboolean isSelected)
{
    PangoLayout *layout = NULL;
    PangoFontDescription *desc;
    int width, height;
    const char *text = shape->params.text;

    if( shape->params.fontName ) {
        layout = pango_cairo_create_layout(cr);
        pango_layout_set_text(layout, text ? text : "", -1);
        desc = pango_font_description_from_string(shape->params.fontName);
        if( zoom != 1.0 ) {
            pango_font_description_set_size(desc,
                    pango_font_description_get_size(desc) * zoom);
        }
        pango_layout_set_font_description (layout, desc);
        pango_font_description_free(desc);
        pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
        pango_layout_get_pixel_size(layout, &width, &height);
    }else{
        width = zoom;
        height = 8 * zoom;
    }
    if( shape->params.fillColor.alpha != 0.0 || isSelected ) {
        cairo_rectangle(cr,
                zoom * (shape->xRight - shape->params.thickness) - 0.5 * width,
                zoom * (shape->yBottom - shape->params.thickness) - 0.5*height,
                width + 2.0 * zoom * shape->params.thickness,
                height + 2.0 * zoom * shape->params.thickness);
        if( shape->params.fillColor.alpha != 0.0 ) {
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
        cairo_move_to(cr, zoom * shape->xRight - 0.5 * width,
                zoom * shape->yBottom - 0.5 * height);
        pango_cairo_show_layout(cr, layout);
        g_object_unref(layout);
        /* pango_cairo_show_layout does not clear path */
        cairo_new_path(cr);
    }
    shape->drawnTextWidth = width / zoom;
    shape->drawnTextHeight = height / zoom;
}

void shape_draw(Shape *shape, cairo_t *cr, gdouble zoom, gboolean isSelected,
        gboolean isCurrent)
{
    switch( shape->type ) {
    case ST_FREEFORM:
        if( shape->ptCount > 0 ) {
            cairo_move_to(cr, zoom * shape->xLeft, zoom * shape->yTop);
            for(int j = 0; j < shape->ptCount; ++j)
                cairo_line_to(cr, zoom * (shape->xLeft + shape->path[j].x),
                        zoom * (shape->yTop + shape->path[j].y));
        }else{
            sd_pathPoint(cr, zoom * shape->xLeft, zoom * shape->yTop);
        }
        strokeShape(shape, cr, zoom, isSelected);
        break;
    case ST_LINE:
        if( shape->xRight != shape->xLeft || shape->yBottom != shape->yTop ) {
            sd_pathLine(cr, zoom * shape->xLeft, zoom * shape->yTop,
                    zoom * shape->xRight, zoom * shape->yBottom,
                    shape->params.angle, zoom * shape->params.round);
        }else{
            sd_pathPoint(cr, zoom * shape->xLeft, zoom * shape->yTop);
        }
        strokeShape(shape, cr, zoom, isSelected);
        if( isCurrent && isSelected )
            strokeResizePoints(shape, cr, zoom, FALSE);
        break;
    case ST_TRIANGLE:
        if( shape->xRight != shape->xLeft || shape->yBottom != shape->yTop ) {
            sd_pathTriangle(cr, zoom * shape->xLeft, zoom * shape->yTop,
                    zoom * shape->xRight, zoom * shape->yBottom,
                    shape->params.angle, zoom * shape->params.round);
        }else{
            sd_pathPoint(cr, zoom * shape->xLeft, zoom * shape->yTop);
        }
        strokeAndFillShape(shape, cr, zoom, isSelected, FALSE, 2.0 / 3.0);
        if( isCurrent && isSelected )
            strokeResizePoints(shape, cr, zoom, FALSE);
        break;
    case ST_RECT:
        if( shape->xRight != shape->xLeft || shape->yBottom != shape->yTop ) {
            sd_pathRect(cr, zoom * shape->xLeft, zoom * shape->yTop,
                    zoom * shape->xRight, zoom * shape->yBottom,
                    zoom * shape->params.round);
        }else{
            sd_pathPoint(cr, zoom * shape->xLeft, zoom * shape->yTop);
        }
        strokeAndFillShape(shape, cr, zoom, isSelected, TRUE, 0.5);
        if( isCurrent && isSelected )
            strokeResizePoints(shape, cr, zoom, TRUE);
        break;
    case ST_OVAL:
        if( shape->xRight != shape->xLeft || shape->yBottom != shape->yTop ) {
            sd_pathOval(cr, zoom * shape->xLeft, zoom * shape->yTop,
                    zoom * shape->xRight, zoom * shape->yBottom);
        }else{
            sd_pathPoint(cr, zoom * shape->xLeft, zoom * shape->yTop);
        }
        strokeAndFillShape(shape, cr, zoom, isSelected, TRUE, 0.5);
        if( isCurrent && isSelected )
            strokeResizePoints(shape, cr, zoom, TRUE);
        break;
    case ST_TEXT:
        drawText(cr, zoom, shape, isSelected);
        break;
    case ST_ARROW:
        if( shape->xRight != shape->xLeft || shape->yBottom != shape->yTop ) {
            sd_pathArrow(cr, zoom * shape->xLeft, zoom * shape->yTop,
                    zoom * shape->xRight, zoom * shape->yBottom,
                    zoom * fmax(shape->params.thickness, 1.0),
                    2.0 + 6.0 / fmax(shape->params.thickness, 1.0),
                    shape->params.angle);
        }else{
            sd_pathPoint(cr, zoom * shape->xLeft, zoom * shape->yTop);
        }
        gdk_cairo_set_source_rgba (cr, &shape->params.strokeColor);
        cairo_fill(cr);
        if( isSelected ) {
            cairo_move_to(cr, zoom * shape->xLeft, zoom * shape->yTop);
            cairo_line_to(cr, zoom * shape->xRight, zoom * shape->yBottom);
            strokeSelection(cr);
            if( isCurrent )
                strokeResizePoints(shape, cr, zoom, FALSE);
        }
        break;
    }
}

Shape *shape_readFromFile(WlqInFile *inFile, gchar **errLoc)
{
    unsigned shapeType, thickness, angle, round, ptCount;
    gdouble xLeft, xRight, yTop, yBottom;
    ShapeParams params;
    Shape *shape;

    params.text = params.fontName = NULL;
    gboolean isOK = wlq_readU32(inFile, &shapeType, errLoc)
            && wlq_readCoordinate(inFile, &xLeft, errLoc)
            && wlq_readCoordinate(inFile, &xRight, errLoc)
            && wlq_readCoordinate(inFile, &yTop, errLoc)
            && wlq_readCoordinate(inFile, &yBottom, errLoc)
            && wlq_readRGBA(inFile, &params.strokeColor, errLoc)
            && wlq_readRGBA(inFile, &params.fillColor, errLoc)
            && wlq_readRGBA(inFile, &params.textColor, errLoc)
            && wlq_readU16(inFile, &thickness, errLoc)
            && wlq_readU16(inFile, &angle, errLoc)
            && wlq_readU16(inFile, &round, errLoc)
            && (params.text = wlq_readString(inFile, errLoc)) != NULL
            && (params.fontName = wlq_readString(inFile, errLoc)) != NULL;
    if( isOK ) {
        params.thickness = thickness;
        params.angle     = angle;
        params.round     = round;
        shape = shape_new(shapeType, xLeft, yTop, &params);
        shape->xRight = xRight;
        shape->yBottom = yBottom;
    }
    g_free((void*)params.text);
    g_free((void*)params.fontName);
    if( isOK ) {
        isOK = wlq_readU32(inFile, &ptCount, errLoc);
        if( isOK && ptCount ) {
            shape->path = g_try_malloc(ptCount * sizeof(*shape->path));
            if( shape->path != NULL ) {
                while( shape->ptCount < ptCount ) {
                    if( ! wlq_readCoordinate(inFile,
                            &shape->path[shape->ptCount].x, errLoc)
                        || ! wlq_readCoordinate(inFile,
                                &shape->path[shape->ptCount].y, errLoc) )
                    {
                        isOK = FALSE;
                        break;
                    }
                    ++shape->ptCount;
                }
            }else{
                isOK = FALSE;
                *errLoc = g_strdup_printf("%s: not enough memory to load "
                        "%u shape points from file",
                        wlq_getInFileName(inFile), ptCount);
            }
        }
        if( ! isOK ) {
            shape_unref(shape);
            shape = NULL;
        }
    }
    return shape;
}

gboolean shape_writeToFile(const Shape *shape, WlqOutFile *outFile,
        gchar **errLoc)
{
    int i;
    gboolean isOK;

    isOK = wlq_writeU32(outFile, shape->type, errLoc)
            && wlq_writeCoordinate(outFile, shape->xLeft, errLoc)
            && wlq_writeCoordinate(outFile, shape->xRight, errLoc)
            && wlq_writeCoordinate(outFile, shape->yTop, errLoc)
            && wlq_writeCoordinate(outFile, shape->yBottom, errLoc)
            && wlq_writeRGBA(outFile, &shape->params.strokeColor, errLoc)
            && wlq_writeRGBA(outFile, &shape->params.fillColor, errLoc)
            && wlq_writeRGBA(outFile, &shape->params.textColor, errLoc)
            && wlq_writeU16(outFile, shape->params.thickness, errLoc)
            && wlq_writeU16(outFile, shape->params.angle, errLoc)
            && wlq_writeU16(outFile, shape->params.round, errLoc)
            && wlq_writeString(outFile, shape->params.text, errLoc)
            && wlq_writeString(outFile, shape->params.fontName, errLoc)
            && wlq_writeU32(outFile, shape->ptCount, errLoc);
    for(i = 0; i < shape->ptCount && isOK; ++i) {
        isOK = wlq_writeCoordinate(outFile, shape->path[i].x, errLoc)
                && wlq_writeCoordinate(outFile, shape->path[i].y, errLoc);
    }
    return isOK;
}

