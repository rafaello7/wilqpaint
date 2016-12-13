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
        cairo_line_to(cr, xEnd, yEnd);
    }else{
        double angleBound, angleBeg, angleEnd, mvIni;
        double lineLen = sqrt((xEnd - xBeg) * (xEnd - xBeg)
                + (yEnd - yBeg) * (yEnd - yBeg));
        double direction = getAngle(xBeg, yBeg, xEnd, yEnd);
        int i, lim = lineLen / movement;
        double fx = (xEnd - xBeg) / lineLen;
        double fy = (yEnd - yBeg) / lineLen;
        double movX = fx * movement;
        double movY = fy * movement;
        double devX = deviation * fy;
        double devY = deviation * fx;
        unsigned isNegFirst = lim & 1;

        if( isNegFirst ) {
            angleBound = fmin(0,
                    asin(0.5 * (lineLen - lim * movement)/round) - angle);
            ++lim;
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
        double xRight, gdouble yBottom, gdouble thickness, gdouble angle)
{
    double xBeg, yBeg, fact;

    if( thickness < 1.0 )
        thickness = 1.0;
    fact = 2.0 + 6.0 / thickness;
    double lineWidth = fmax(thickness, 1.0);
    double minLen;
    double lineLen = sqrt((xRight - xLeft) * (xRight - xLeft)
            + (yBottom - yTop) * (yBottom - yTop));
    double angleTan = tan(angle * G_PI / 360);
    double xEnd = 0.5 * fact * lineWidth / angleTan * (xRight-xLeft) / lineLen;
    double yEnd = 0.5 * fact * lineWidth / angleTan * (yBottom-yTop) / lineLen;
    cairo_move_to(cr, xRight, yBottom);
    cairo_line_to(cr, xRight - xEnd - yEnd * angleTan,
            yBottom - yEnd + xEnd * angleTan);
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
            xRight - (xEnd + yEnd * angleTan)/fact
                 - xBeg * (1 - 1 / fact),
            yBottom - (yEnd - xEnd * angleTan)/fact
                 - yBeg * (1 - 1 / fact));
        cairo_line_to(cr, xLeft - lineWidth * (yBottom - yTop) / lineLen / 2,
                yTop + lineWidth * (xRight - xLeft) / lineLen / 2);
        cairo_line_to(cr, xLeft + lineWidth * (yBottom - yTop) / lineLen / 2,
                yTop - lineWidth * (xRight - xLeft) / lineLen / 2);
        cairo_line_to(cr, xRight - (xEnd - yEnd * angleTan)/fact
                 - xBeg * (1 - 1 / fact),
            yBottom - (yEnd + xEnd * angleTan)/fact - yBeg * (1 - 1 / fact));
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
    round = fmin(round, 0.5 * G_SQRT2 * fmin(xEnd - xBeg, yEnd - yBeg));
    xBeg -= (1 - 0.5 * G_SQRT2) * round;
    yBeg -= (1 - 0.5 * G_SQRT2) * round;
    xEnd += (1 - 0.5 * G_SQRT2) * round;
    yEnd += (1 - 0.5 * G_SQRT2) * round;
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

