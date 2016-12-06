#include <gtk/gtk.h>
#include "shapedrawing.h"
#include <math.h>


void sd_pathLine(cairo_t *cr, gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
}

void sd_pathArrow(cairo_t *cr, gdouble shapeXRef, gdouble shapeYRef,
        gdouble shapeXEnd, gdouble shapeYEnd, gdouble thickness, gint angle)
{
    double xBeg, yBeg, fact;

    if( thickness < 1.0 )
        thickness = 1.0;
    fact = 2.0 + 6.0 / thickness;
    double lineWidth = fmax(thickness, 1.0);
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

void sd_pathTriangle(cairo_t *cr, gdouble xBeg, gdouble yBeg,
        gdouble xEnd, gdouble yEnd, gdouble angle, gdouble round)
{
    double angleTan = tan(angle * G_PI / 360);
    double angleSin = sin(angle * G_PI / 360);
    double height = sqrt((xEnd - xBeg) * (xEnd - xBeg)
                + (yEnd - yBeg) * (yEnd - yBeg));
    if( round == 0 ) {
        cairo_move_to(cr, xBeg, yBeg);
        cairo_line_to(cr, xEnd + (yEnd - yBeg) * angleTan,
                yEnd - (xEnd - xBeg) * angleTan);
        cairo_line_to(cr, xEnd - (yEnd - yBeg) * angleTan,
                yEnd + (xEnd - xBeg) * angleTan);
        cairo_close_path(cr);
    }else if( round >= height * (1.0 - 1.0 / (1.0 + angleSin)) ) {
        double fact = 1.0 / (1.0 + angleSin);
        cairo_arc(cr, xBeg + (xEnd - xBeg) * fact,
                yBeg + (yEnd - yBeg) * fact,
                height * (1.0 - fact), 0, 2 * G_PI);
    }else{
        double x1 = xEnd - xBeg + (yEnd - yBeg) * angleTan;
        double y1 = yEnd - yBeg - (xEnd - xBeg) * angleTan;
        double x2 = xEnd - xBeg - (yEnd - yBeg) * angleTan;
        double y2 = yEnd - yBeg + (xEnd - xBeg) * angleTan;
        drawArc(cr, xBeg + x2, yBeg + y2, xBeg, yBeg, angle, round);
        drawArc(cr, xBeg, yBeg, xBeg + x1, yBeg + y1, 90 - angle / 2, round);
        drawArc(cr, xBeg + x1, yBeg + y1, xBeg + x2, yBeg + y2,
                90 - angle / 2, round);
        cairo_close_path(cr);
    }
}

void sd_pathRect(cairo_t *cr, gdouble xBeg, gdouble yBeg,
        gdouble xEnd, gdouble yEnd, gdouble round)
{
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
    round = fmin(round, 0.5 * fmin(xEnd - xBeg, yEnd - yBeg));
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
}

void sd_pathOval(cairo_t *cr, gdouble xBeg, gdouble yBeg,
        gdouble xEnd, gdouble yEnd)
{
    cairo_matrix_t matrix;

    matrix.xx = matrix.yy = 1.0;
    matrix.xy = matrix.yx = matrix.x0 = matrix.y0 = 0;
    cairo_save(cr);
    if( fabs(xEnd - xBeg) < ABS(yEnd - yBeg) ) {
        double xMid = 0.5 * (xBeg + xEnd);
        matrix.xx = fabs((xEnd - xBeg) / (yEnd - yBeg));
        matrix.x0 = xMid * (1.0 - matrix.xx);
    }else{
        double yMid = 0.5 * (yBeg + yEnd);
        matrix.yy = fabs((yEnd - yBeg) / (xEnd - xBeg));
        matrix.y0 = yMid * (1.0 - matrix.yy);
    }
    cairo_transform(cr, &matrix);
    cairo_arc(cr, 0.5 * (xBeg + xEnd), 0.5 * (yBeg + yEnd),
            0.5 * fmax(fabs(xEnd - xBeg), fabs(yEnd - yBeg)),
            0, 2 * G_PI);
    cairo_restore(cr);
}

