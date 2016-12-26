#include <gtk/gtk.h>
#include "shapedrawing.h"
#include <math.h>

static double getAngle(gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
    double res;

    if( y1 == y2 ) {
        res = x2 > x1 ? 0 : G_PI;
    }else{
        res = G_PI / 2.0 - atan((x2 - x1)/ (y2 - y1));
        if( y2 < y1 )
            res += G_PI;
    }
    return res;
}

/* cairo bug workaround: cairo does not cope with long lines
 */
static void crLineTo(cairo_t *cr, gdouble x, gdouble y)
{
    gdouble xCur, yCur;

    cairo_get_current_point(cr, &xCur, &yCur);
    if( fabs(x - xCur) > fabs(y - yCur) ) {
        double dy = (y - yCur) * 10000 / (x - xCur);
        if( x > xCur ) {
            while( x - xCur > 10000 ) {
                xCur += 10000;
                yCur += dy;
                cairo_line_to(cr, xCur, yCur);
            }
        }else{
            while( xCur - x > 10000 ) {
                xCur -= 10000;
                yCur -= dy;
                cairo_line_to(cr, xCur, yCur);
            }
        }
    }else{
        double dx = (x - xCur) * 10000 / (y - yCur);
        if( y > yCur ) {
            while( y - yCur > 10000 ) {
                xCur += dx;
                yCur += 10000;
                cairo_line_to(cr, xCur, yCur);
            }
        }else{
            while( yCur - y > 10000 ) {
                xCur -= dx;
                yCur -= 10000;
                cairo_line_to(cr, xCur, yCur);
            }
        }
    }
    cairo_line_to(cr, x, y);
}

void sd_pathPoint(cairo_t *cr, gdouble x, gdouble y)
{
    cairo_arc(cr, x, y, 1, 0, 2 * G_PI);
}

void sd_pathLine(cairo_t *cr, gdouble xBeg, gdouble yBeg,
        gdouble xEnd, gdouble yEnd, gdouble angle, gdouble round)
{
    gdouble movement, deviation;

    angle *= G_PI / 360;
    movement = 2.0 * round * sin(angle);
    deviation = 2.0 * round * cos(angle);
    //printf("movement: %f, deviation: %f\n", movement, 2 * round - deviation);
    /* don't allow too dense wavy line from performance reasons */
    if( movement < 2.0 || 2.0 * round - deviation < 0.5 ) {
        cairo_move_to(cr, xBeg, yBeg);
        crLineTo(cr, xEnd, yEnd);
    }else{
        double angleBound, angleBeg, angleEnd, mvIni, lim;
        double lineLen = sqrt((xEnd - xBeg) * (xEnd - xBeg)
                + (yEnd - yBeg) * (yEnd - yBeg));
        double direction = getAngle(xBeg, yBeg, xEnd, yEnd);
        int i, isNegFirst;
        double fx = (xEnd - xBeg) / lineLen, fy = (yEnd - yBeg) / lineLen;
        double movX = fx * movement, movY = fy * movement;
        double devX = deviation * fy, devY = deviation * fx;

        isNegFirst = modf(lineLen / movement, &lim) <= 0.5 && lim != 0.0;
        if( fmod(lim, 2.0) == 1 ) {
            angleBound = fmin(0,
                    asin(0.5 * (lineLen - lim * movement)/round) - angle);
            ++lim;
            isNegFirst = 1;
        }else{
            angleBound = fmin(angle,
                    asin(0.5 * (lineLen - lim * movement)/round));
        }
        xBeg -= round * fy;
        yBeg += round * fx;
        mvIni = 0.5 * (lineLen/movement - lim);
        for(i = 0; i <= lim; ++i) {
            double xCur = xBeg + (i + mvIni) * movX;
            double yCur = yBeg + (i + mvIni) * movY;
            angleBeg = angleEnd = angle;

            if( i == 0 )
                angleBeg = angleBound;
            if( i == lim )
                angleEnd = angleBound;
            if( (i & 1) == isNegFirst ) {
                cairo_arc(cr, xCur, yCur, round,
                        direction - 0.5 * G_PI - angleBeg,
                        direction - 0.5 * G_PI + angleEnd);
            }else{
                cairo_arc_negative(cr, xCur + devX, yCur - devY, round,
                        direction + 0.5 * G_PI + angleBeg,
                        direction + 0.5 * G_PI - angleEnd);
            }
        }
    }
}

void sd_pathArrow(cairo_t *cr, gdouble xLeft, gdouble yTop,
        double xRight, gdouble yBottom, gdouble thickness, gdouble proportion,
        gdouble angle)
{
    double xBeg, yBeg;

    if( thickness < 1.0 )
        thickness = 1.0;
    double minLen;
    double lineLen = sqrt((xRight - xLeft) * (xRight - xLeft)
            + (yBottom - yTop) * (yBottom - yTop));
    double angleTan = tan(angle * G_PI / 360);
    double xEnd = 0.5 * proportion * thickness / angleTan
        * (xRight-xLeft) / lineLen;
    double yEnd = 0.5 * proportion * thickness / angleTan
        * (yBottom-yTop) / lineLen;
    cairo_move_to(cr, xRight, yBottom);
    cairo_line_to(cr, xRight - xEnd - yEnd * angleTan,
            yBottom - yEnd + xEnd * angleTan);
    if( angle < 90 ) {
        xBeg = xEnd * 0.5 * (1 + angleTan * angleTan);
        yBeg = yEnd * 0.5 * (1 + angleTan * angleTan);
        minLen = 0.5 * thickness * ((0.5 * proportion + 0.5) / angleTan
                + (0.5 * proportion - 0.5) * angleTan);
    }else{
        xBeg = xEnd;
        yBeg = yEnd;
        minLen = 0.5 * proportion * thickness / angleTan;
    }
    if( lineLen > minLen ) {
        cairo_line_to(cr,
            xRight - (xEnd + yEnd * angleTan)/proportion
                 - xBeg * (1 - 1 / proportion),
            yBottom - (yEnd - xEnd * angleTan)/proportion
                 - yBeg * (1 - 1 / proportion));
        cairo_line_to(cr, xLeft - thickness * (yBottom - yTop) / lineLen / 2,
                yTop + thickness * (xRight - xLeft) / lineLen / 2);
        cairo_line_to(cr, xLeft + thickness * (yBottom - yTop) / lineLen / 2,
                yTop - thickness * (xRight - xLeft) / lineLen / 2);
        cairo_line_to(cr, xRight - (xEnd - yEnd * angleTan)/proportion
                - xBeg * (1 - 1 / proportion),
                yBottom - (yEnd + xEnd * angleTan)/proportion
                - yBeg * (1 - 1 / proportion));
    }else if( angle < 90 ) {
        cairo_line_to(cr, xRight - xBeg, yBottom - yBeg);
    }
    cairo_line_to(cr, xRight - xEnd + yEnd * angleTan,
            yBottom - yEnd - xEnd * angleTan);
    cairo_close_path(cr);
}

void sd_pathTriangle(cairo_t *cr, gdouble xBeg, gdouble yBeg,
        gdouble xEnd, gdouble yEnd, gdouble angle, gdouble round)
{
    double angleTan = tan(angle * G_PI / 360);
    double height = sqrt((xEnd - xBeg) * (xEnd - xBeg)
                + (yEnd - yBeg) * (yEnd - yBeg));
    if( round == 0 ) {
        cairo_move_to(cr, xBeg, yBeg);
        cairo_line_to(cr, xEnd + (yEnd - yBeg) * angleTan,
                yEnd - (xEnd - xBeg) * angleTan);
        cairo_line_to(cr, xEnd - (yEnd - yBeg) * angleTan,
                yEnd + (xEnd - xBeg) * angleTan);
        cairo_close_path(cr);
    }else if( round >= 0.5 * height ) {
        cairo_arc(cr, 0.5 * (xBeg + xEnd), 0.5 * (yBeg + yEnd),
                0.5 * height, 0, 2 * G_PI);
    }else{
        gdouble xAdd = (xEnd - xBeg) * round / height;
        gdouble yAdd = (yEnd - yBeg) * round / height;
        double lineAngle = getAngle(xEnd - (yEnd - yBeg) * angleTan,
                yEnd + (xEnd - xBeg) * angleTan, xBeg, yBeg);
        cairo_arc(cr, xBeg + xAdd, yBeg + yAdd, round, lineAngle - G_PI/2,
                lineAngle + G_PI/2 - angle * G_PI / 180);
        lineAngle = getAngle(xBeg, yBeg, xEnd + (yEnd - yBeg) * angleTan,
                yEnd - (xEnd - xBeg) * angleTan);
        cairo_arc(cr, xEnd - xAdd + (yEnd - yBeg - 2 * yAdd) * angleTan,
                yEnd - yAdd - (xEnd - xBeg - 2 * xAdd) * angleTan, round,
                lineAngle - G_PI/2,
                lineAngle + G_PI/2 - (90 - angle / 2) * G_PI / 180);
        lineAngle = getAngle(xEnd + (yEnd - yBeg) * angleTan,
                yEnd - (xEnd - xBeg) * angleTan,
                xEnd - (yEnd - yBeg) * angleTan,
                yEnd + (xEnd - xBeg) * angleTan);
        cairo_arc(cr, xEnd - xAdd - (yEnd - yBeg - 2 * yAdd) * angleTan,
                yEnd - yAdd + (xEnd - xBeg - 2 * xAdd) * angleTan, round,
                lineAngle - G_PI/2,
                lineAngle + G_PI/2 - (90 - angle / 2) * G_PI / 180);
        cairo_close_path(cr);
    }
}

void sd_pathRect(cairo_t *cr, gdouble xBeg, gdouble yBeg,
        gdouble xEnd, gdouble yEnd, gdouble round, gdouble angle)
{
    gdouble angleSin, angleCos, xLen, yLen, xMid1, yMid1, xMid3, yMid3, tmp;

    angle *= G_PI / 180;
    angleSin = sin(angle);
    angleCos = cos(angle);
    xLen = (xEnd - xBeg) * angleCos - (yEnd - yBeg) * angleSin;
    yLen = (xEnd - xBeg) * angleSin + (yEnd - yBeg) * angleCos;
    xMid1 = xBeg + xLen * angleCos;
    yMid1 = yBeg - xLen * angleSin;
    xMid3 = xEnd - xLen * angleCos;
    yMid3 = yEnd + xLen * angleSin;
    if( xLen < 0 && yLen < 0 ) {
        tmp = xBeg; xBeg = xEnd; xEnd = tmp;
        tmp = yBeg; yBeg = yEnd; yEnd = tmp;
        tmp = xMid1; xMid1 = xMid3; xMid3 = tmp;
        tmp = yMid1; yMid1 = yMid3; yMid3 = tmp;
        xLen = -xLen;
        yLen = -yLen;
    }else if( xLen < 0 ) {
        tmp = xBeg; xBeg = xMid1; xMid1 = tmp;
        tmp = yBeg; yBeg = yMid1; yMid1 = tmp;
        tmp = xEnd; xEnd = xMid3; xMid3 = tmp;
        tmp = yEnd; yEnd = yMid3; yMid3 = tmp;
        xLen = -xLen;
    }else if( yLen < 0 ) {
        tmp = xBeg; xBeg = xMid3; xMid3 = tmp;
        tmp = yBeg; yBeg = yMid3; yMid3 = tmp;
        tmp = xEnd; xEnd = xMid1; xMid1 = tmp;
        tmp = yEnd; yEnd = yMid1; yMid1 = tmp;
        yLen = -yLen;
    }
    round = fmin(round, 0.5 * G_SQRT2 * fmin(xLen, yLen));
    xBeg -= (1 - 0.5 * G_SQRT2) * round * (angleCos + angleSin);
    yBeg -= (1 - 0.5 * G_SQRT2) * round * (angleCos - angleSin);
    xMid1 += (1 - 0.5 * G_SQRT2) * round * (angleCos - angleSin);
    yMid1 -= (1 - 0.5 * G_SQRT2) * round * (angleCos + angleSin);
    xEnd += (1 - 0.5 * G_SQRT2) * round * (angleCos + angleSin);
    yEnd += (1 - 0.5 * G_SQRT2) * round * (angleCos - angleSin);
    xMid3 -= (1 - 0.5 * G_SQRT2) * round * (angleCos - angleSin);
    yMid3 += (1 - 0.5 * G_SQRT2) * round * (angleCos + angleSin);
    cairo_move_to(cr, xBeg + round * angleSin, yBeg + round * angleCos);
    if( round )
        cairo_arc(cr, xBeg + round * (angleSin + angleCos),
                yBeg + round * (angleCos - angleSin), round,
                G_PI - angle, 1.5 * G_PI - angle);
    cairo_line_to(cr, xMid1 - round * angleCos, yMid1 + round * angleSin);
    if( round )
        cairo_arc(cr, xMid1 + round * (angleSin - angleCos),
                yMid1 + round * (angleSin + angleCos), round,
                1.5 * G_PI - angle, 2 * G_PI - angle);
    cairo_line_to(cr, xEnd - round * angleSin, yEnd - round * angleCos);
    if( round )
        cairo_arc(cr, xEnd - round * (angleSin + angleCos),
                yEnd - round * (angleCos - angleSin),
                round, -angle, 0.5 * G_PI - angle);
    cairo_line_to(cr, xMid3 + round * angleCos, yMid3 - round * angleSin);
    if( round )
        cairo_arc(cr, xMid3 + round * (angleCos - angleSin),
                yMid3 - round * (angleSin + angleCos), round,
                0.5 * G_PI - angle, G_PI - angle);
    cairo_close_path(cr);
}

void sd_pathOval(cairo_t *cr, gdouble xBeg, gdouble yBeg,
        gdouble xEnd, gdouble yEnd)
{
    cairo_matrix_t matrix;

    if( xEnd != xBeg && yEnd != yBeg ) {
        cairo_save(cr);
        if( fabs(xEnd - xBeg) != fabs(yEnd - yBeg) ) {
            matrix.xx = matrix.yy = 1.0;
            matrix.xy = matrix.yx = matrix.x0 = matrix.y0 = 0;
            if( fabs(xEnd - xBeg) < fabs(yEnd - yBeg) ) {
                double xMid = 0.5 * (xBeg + xEnd);
                matrix.xx = fabs((xEnd - xBeg) / (yEnd - yBeg));
                matrix.x0 = xMid * (1.0 - matrix.xx);
            }else{
                double yMid = 0.5 * (yBeg + yEnd);
                matrix.yy = fabs((yEnd - yBeg) / (xEnd - xBeg));
                matrix.y0 = yMid * (1.0 - matrix.yy);
            }
            cairo_transform(cr, &matrix);
        }
        cairo_arc(cr, 0.5 * (xBeg + xEnd), 0.5 * (yBeg + yEnd),
                0.5 * G_SQRT2 * fmax(fabs(xEnd - xBeg), fabs(yEnd - yBeg)),
                0, 2 * G_PI);
        cairo_restore(cr);
    }else{
        cairo_rectangle(cr,
                xBeg - 0.5 * (G_SQRT2 - 1) * (xEnd - xBeg),
                yBeg - 0.5 * (G_SQRT2 - 1) * (yEnd - yBeg),
                G_SQRT2 * (xEnd - xBeg), G_SQRT2 * (yEnd - yBeg));
    }
}

