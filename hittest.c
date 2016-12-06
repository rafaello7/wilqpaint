#include <gtk/gtk.h>
#include "hittest.h"
#include <math.h>

static gdouble ptDistSqr(double x1, double y1, double x2, double y2)
{
    return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
}

static void swapEnds(gdouble *x1, gdouble *y1, gdouble *x2, gdouble *y2)
{
    gdouble t;

    t = *x1; *x1 = *x2; *x2 = t;
    t = *y1; *y1 = *y2; *y2 = t;
}

/* Assume the two sections are lying on one line.
 * Returns TRUE when the two sections have common at least one point.
 */
static gboolean sectionsOverlap(
        gdouble s1x1, gdouble s1y1, gdouble s1x2, gdouble s1y2,
        gdouble s2x1, gdouble s2y1, gdouble s2x2, gdouble s2y2)
{
    return fmin(s1x1, s1x2) <= fmax(s2x1, s2x2)
        && fmax(s1x1, s1x2) >= fmin(s2x1, s2x2)
        && fmin(s1y1, s1y2) <= fmax(s2y1, s2y2)
        && fmax(s1y1, s1y2) >= fmin(s2y1, s2y2);
}

/* In functions below a "half-plane" is specified by two points and a
 * thickness parameter. These points, (lx1, ly1), (lx2, ly2) are defining
 * a line going through these points. The half-plane boundary is a line
 * parallel to this line, as shown below:
 *
 *
 *          * points beyond half-plane (line left side)
 *
 *  >------------------- the half-plane boundary --------------------------->
 *                                                       ^
 *          * points on half-plane (line right side)     | 0.5 * thickness
 *                                                       v
 *  (lx1, ly1) .-------------------------------------------------. (lx2, ly2)
 *
 * The half-plane boundary is a part of the half-plane.
 */

static gboolean halfplaneHitTest(gdouble lx1, gdouble ly1,
        gdouble lx2, gdouble ly2, gdouble thickness,
        gdouble rx1, gdouble ry1, gdouble rx2, gdouble ry2)
{
    gdouble lenSqr = (lx2 - lx1) * (lx2 - lx1) + (ly2 - ly1) * (ly2 - ly1);
    gdouble len = sqrt(lenSqr);
    gdouble a = lx1 * ly2 - lx2 * ly1 + 0.5 * thickness * len;
    gdouble dx = lx1 - lx2, dy = ly2 - ly1;

    return fmin(rx1 * dy, rx2 * dy) + fmin(ry1 * dx, ry2 * dx) < a;
}

static gboolean getSubsectionLyingOnHalfplane(
        gdouble lx1, gdouble ly1, gdouble lx2, gdouble ly2, gdouble thickness,
        gdouble *sx1, gdouble *sy1, gdouble *sx2, gdouble *sy2)
{
    gdouble d = 0.5 * thickness * sqrt(ptDistSqr(lx1, ly1, lx2, ly2));
    gdouble sxbeg = *sx1, sybeg = *sy1, sxend = *sx2, syend = *sy2;
    gdouble a1 = ly2 - ly1, b1 = lx1 - lx2, c1 = lx2 * ly1 - lx1 * ly2 - d;
    gboolean res = TRUE;

    if( fabs(sxend - sxbeg) > fabs(syend - sybeg) ) {
        if( sxbeg > sxend )
            swapEnds(&sxbeg, &sybeg, &sxend, &syend);
        gdouble a2 = syend - sybeg, b2 = sxbeg - sxend;
        gdouble c2 = sxend * sybeg - sxbeg * syend;
        gdouble f = a1 * b2 - a2 * b1, g = b1 * c2 - b2 * c1;
        /* x: x * f >= g, sxbeg <= x <= sxend */
        if( f >= 0 ) {
            if( sxend * f >= g ) {
                if( sxbeg * f < g ) {
                    sxbeg = g / f;
                    if( a2 != 0 )
                        sybeg = - (c2 + a2 * sxbeg) / b2;
                }
            }else
                res = FALSE;
        }else{
            if( sxbeg * f >= g ) {
                if( sxend * f < g ) {
                    sxend = g / f;
                    if( a2 != 0 )
                        syend = - (c2 + a2 * sxend) / b2;
                }
            }else
                res = FALSE;
        }
    }else if( syend != sybeg ) {
        gdouble isYrev = sxend != sxbeg && (sxend < sxbeg) != (syend < sybeg);
        if( sybeg > syend )
            swapEnds(&sxbeg, &sybeg, &sxend, &syend);
        gdouble a2 = syend - sybeg, b2 = sxbeg - sxend;
        gdouble c2 = sxend * sybeg - sxbeg * syend;
        gdouble f = a1 * b2 - a2 * b1, g = a2 * c1 - a1 * c2;
        /* y: y * f >= g, sybeg <= y <= syend */
        if( f >= 0 ) {
            if( syend * f >= g ) {
                if( sybeg * f < g ) {
                    sybeg = g / f;
                    if( b2 != 0 )
                        sxbeg = - (b2 * sybeg + c2) / a2;
                }
            }else
                res = FALSE;
        }else{
            if( sybeg * f >= g ) {
                if( syend * f < g ) {
                    syend = g / f;
                    if( b2 != 0 )
                        sxend = - (b2 * syend + c2) / a2;
                }
            }else
                res = FALSE;
        }
        if( isYrev ) /* ensure sxbeg <= sxend */
            swapEnds(&sxbeg, &sybeg, &sxend, &syend);
    }else{
        /* sxbeg == sxend && sybeg == syend */
        res = a1 * sxbeg + b1 * sybeg + c1 <= 0;
    }
    if( res ) {
        *sx1 = sxbeg;
        *sy1 = sybeg;
        *sx2 = sxend;
        *sy2 = syend;
    }
    return res;
}

/* The half-plane is ortogonal to line defined by points (lx1, ly1) and
 * (lx2, ly2). The half-plane boundary goes through (lx1, ly1).
 */
static gboolean getSubsectionLyingOnHalfplaneOrtogonal(
        gdouble lx1, gdouble ly1, gdouble lx2, gdouble ly2,
        gdouble *sx1, gdouble *sy1, gdouble *sx2, gdouble *sy2)
{
    gdouble sxbeg = *sx1, sybeg = *sy1, sxend = *sx2, syend = *sy2;
    gdouble a1 = lx1 - lx2, b1 = ly1 - ly2;
    gdouble c1 = lx1 * (lx2 - lx1) + ly1 * (ly2 - ly1);
    gboolean res = TRUE;

    if( fabs(sxend - sxbeg) > fabs(syend - sybeg) ) {
        if( sxbeg > sxend )
            swapEnds(&sxbeg, &sybeg, &sxend, &syend);
        gdouble a2 = syend - sybeg, b2 = sxbeg - sxend;
        gdouble c2 = sxend * sybeg - sxbeg * syend;
        gdouble f = a1 * b2 - a2 * b1, g = b1 * c2 - b2 * c1;
        /* x: x * f >= g, sxbeg <= x <= sxend */
        if( f >= 0 ) {
            if( sxend * f >= g ) {
                if( sxbeg * f < g ) {
                    sxbeg = g / f;
                    if( a2 != 0 )
                        sybeg = - (c2 + a2 * sxbeg) / b2;
                }
            }else
                res = FALSE;
        }else{
            if( sxbeg * f >= g ) {
                if( sxend * f < g ) {
                    sxend = g / f;
                    if( a2 != 0 )
                        syend = - (c2 + a2 * sxend) / b2;
                }
            }else
                res = FALSE;
        }
    }else if( syend != sybeg ) {
        gdouble isYrev = sxend != sxbeg && (sxend < sxbeg) != (syend < sybeg);
        if( sybeg > syend )
            swapEnds(&sxbeg, &sybeg, &sxend, &syend);
        gdouble a2 = syend - sybeg, b2 = sxbeg - sxend;
        gdouble c2 = sxend * sybeg - sxbeg * syend;
        gdouble f = a1 * b2 - a2 * b1, g = a2 * c1 - a1 * c2;
        /* y: y * f >= g, sybeg <= y <= syend */
        if( f >= 0 ) {
            if( syend * f >= g ) {
                if( sybeg * f < g ) {
                    sybeg = g / f;
                    if( b2 != 0 )
                        sxbeg = - (b2 * sybeg + c2) / a2;
                }
            }else
                res = FALSE;
        }else{
            if( sybeg * f >= g ) {
                if( syend * f < g ) {
                    syend = g / f;
                    if( b2 != 0 )
                        sxend = - (b2 * syend + c2) / a2;
                }
            }else
                res = FALSE;
        }
        if( isYrev ) /* ensure sxbeg <= sxend */
            swapEnds(&sxbeg, &sybeg, &sxend, &syend);
    }else{
        /* sxbeg == sxend && sybeg == syend */
        /*
        printf("getSubsectionLyingOnHalfplaneOrtogonal: "
                "line=((%.0f, %.0f), (%.0f, %.0f))\n", lx1, ly1, lx2, ly2);
        printf("    pt=(%.0f, %.0f), a1=%f, b1=%f, c1=%f, res=%f\n",
                sxbeg, sybeg, a1, b1, c1, a1 * sxbeg + b1 * sybeg + c1);
                */
        res = a1 * sxbeg + b1 * sybeg + c1 <= 0;
    }
    if( res ) {
        *sx1 = sxbeg;
        *sy1 = sybeg;
        *sx2 = sxend;
        *sy2 = syend;
    }
    return res;
}

static gboolean getSubsectionLyingOnLine(
        gdouble lx1, gdouble ly1, gdouble lx2, gdouble ly2, gdouble thickness,
        gdouble *sx1, gdouble *sy1, gdouble *sx2, gdouble *sy2)
{
    gdouble d = 0.5 * thickness * sqrt(ptDistSqr(lx1, ly1, lx2, ly2));
    gdouble sxbeg = *sx1, sybeg = *sy1, sxend = *sx2, syend = *sy2;
    gdouble a1 = ly2 - ly1, b1 = lx1 - lx2, c1 = lx2 * ly1 - lx1 * ly2;
    gboolean res = TRUE;

    /*
    printf("getSubsectionLyingOnLine: line=((%.0f, %.0f), (%.0f, %.0f))\n",
            lx1, ly1, lx2, ly2);
    printf("                          sect=((%.0f, %.0f), (%.0f, %.0f))\n",
            *sx1, *sy1, *sx2, *sy2);
            */
    if( fabs(sxend - sxbeg) > fabs(syend - sybeg) ) {
        if( sxbeg > sxend )
            swapEnds(&sxbeg, &sybeg, &sxend, &syend);
        gdouble a2 = syend - sybeg, b2 = sxbeg - sxend;
        gdouble c2 = sxend * sybeg - sxbeg * syend;
        d *= -b2;
        gdouble f = a1 * b2 - a2 * b1, g = b1 * c2 - b2 * c1;
        if( f >= 0 ) {
            if( sxend * f >= g - d && sxbeg * f <= g + d ) {
                if( sxbeg * f < g - d ) {
                    sxbeg = (g - d) / f;
                    if( a2 != 0 )
                        sybeg = - (c2 + a2 * sxbeg) / b2;
                }
                if( sxend * f > g + d ) {
                    sxend = (g + d) / f;
                    if( a2 != 0 )
                        syend = - (c2 + a2 * sxend) / b2;
                }
            }else
                res = FALSE;
        }else{
            if( sxbeg * f >= g - d && sxend * f <= g + d ) {
                if( sxend * f < g - d ) {
                    sxend = (g - d) / f;
                    if( a2 != 0 )
                        syend = - (c2 + a2 * sxend) / b2;
                }
                if( sxbeg * f > g + d ) {
                    sxbeg = (g + d) / f;
                    if( a2 != 0 )
                        sybeg = - (c2 + a2 * sxbeg) / b2;
                }
            }else
                res = FALSE;
        }
    }else if( syend != sybeg ) {
        gdouble isYrev = sxend != sxbeg && (sxend < sxbeg) != (syend < sybeg);
        if( sybeg > syend )
            swapEnds(&sxbeg, &sybeg, &sxend, &syend);
        gdouble a2 = syend - sybeg, b2 = sxbeg - sxend;
        gdouble c2 = sxend * sybeg - sxbeg * syend;
        d *= a2;
        gdouble f = a1 * b2 - a2 * b1, g = a2 * c1 - a1 * c2;
        if( f >= 0 ) {
            if( syend * f >= g - d && sybeg * f <= g + d ) {
                if( sybeg * f < g - d ) {
                    sybeg = (g - d) / f;
                    if( b2 != 0 )
                        sxbeg = - (b2 * sybeg + c2) / a2;
                }
                if( syend * f > g + d ) {
                    syend = (g + d) / f;
                    if( b2 != 0 )
                        sxend = - (b2 * syend + c2) / a2;
                }
            }else
                res = FALSE;
        }else{
            if( syend * f <= g + d && sybeg * f >= g - d ) {
                if( sybeg * f > g + d ) {
                    sybeg = (g + d) / f;
                    if( b2 != 0 )
                        sxbeg = - (b2 * sybeg + c2) / a2;
                }
                if( syend * f < g - d ) {
                    syend = (g - d) / f;
                    if( b2 != 0 )
                        sxend = - (b2 * syend + c2) / a2;
                }
            }else
                res = FALSE;
        }
        if( isYrev ) /* ensure sxbeg <= sxend */
            swapEnds(&sxbeg, &sybeg, &sxend, &syend);
    }else{
        /* sxbeg == sxend && sybeg == syend */
        res = fabs(a1 * sxbeg + b1 * sybeg + c1) <= d;
    }
    if( res ) {
        *sx1 = sxbeg;
        *sy1 = sybeg;
        *sx2 = sxend;
        *sy2 = syend;
    }
    return res;
}

/* Checks whether section touches intersection of two half-planes.
 */
static gboolean isSectionOnHalfplaneIntersection(
        gdouble l1x1, gdouble l1y1, gdouble l1x2, gdouble l1y2,
        gdouble l2x1, gdouble l2y1, gdouble l2x2, gdouble l2y2,
        gdouble thickness, gdouble sx1, gdouble sy1, gdouble sx2, gdouble sy2)
{
    gdouble sl1x1 = sx1, sl1x2 = sx2, sl2x1 = sx1, sl2x2 = sx2;
    gdouble sl1y1 = sy1, sl1y2 = sy2, sl2y1 = sy1, sl2y2 = sy2;

    /*
    printf("        isSectionOnHalfplaneIntersection: "
            "line1=((%.0f, %.0f), (%.0f, %.0f))\n",
            l1x1, l1y1, l1x2, l1y2);
    printf("                      line2=((%.0f, %.0f), (%.0f, %.0f))\n",
            l2x1, l2y1, l2x2, l2y2);
    printf("                      sect=((%.0f, %.0f), (%.0f, %.0f))\n",
            sx1, sy1, sx2, sy2);
            */
    return getSubsectionLyingOnHalfplane(l1x1, l1y1, l1x2, l1y2,
            thickness, &sl1x1, &sl1y1, &sl1x2, &sl1y2)
        && getSubsectionLyingOnHalfplane(l2x1, l2y1, l2x2, l2y2,
            thickness, &sl2x1, &sl2y1, &sl2x2, &sl2y2)
        && sectionsOverlap(sl1x1, sl1y1, sl1x2, sl1y2,
                sl2x1, sl2y1, sl2x2, sl2y2);
}

/* Checks whether rectangle overlaps intersection of two half-planes.
 */
static gboolean twoHalfplanesIntersectionHitTest(
        gdouble l1x1, gdouble l1y1, gdouble l1x2, gdouble l1y2,
        gdouble l2x1, gdouble l2y1, gdouble l2x2, gdouble l2y2,
        gdouble thickness, gdouble rx1, gdouble ry1, gdouble rx2, gdouble ry2)
{
    /*
    printf("twoHalfplanesHitTest: line1=((%.0f, %.0f), (%.0f, %.0f))\n",
            l1x1, l1y1, l1x2, l1y2);
    printf("                      line2=((%.0f, %.0f), (%.0f, %.0f))\n",
            l2x1, l2y1, l2x2, l2y2);
    printf("                      rect =((%.0f, %.0f), (%.0f, %.0f))\n",
            rx1, ry1, rx2, ry2);
            */
    return isSectionOnHalfplaneIntersection(l1x1, l1y1, l1x2, l1y2,
            l2x1, l2y1, l2x2, l2y2, thickness, rx1, ry1, rx2, ry1)
        || isSectionOnHalfplaneIntersection(l1x1, l1y1, l1x2, l1y2,
            l2x1, l2y1, l2x2, l2y2, thickness, rx1, ry2, rx2, ry2)
        || isSectionOnHalfplaneIntersection(l1x1, l1y1, l1x2, l1y2,
            l2x1, l2y1, l2x2, l2y2, thickness, rx1, ry1, rx1, ry2)
        || isSectionOnHalfplaneIntersection(l1x1, l1y1, l1x2, l1y2,
            l2x1, l2y1, l2x2, l2y2, thickness, rx2, ry1, rx2, ry2);
}

static gboolean lineHitTest(gdouble lx1, gdouble ly1, gdouble lx2, gdouble ly2,
        gdouble thickness, gdouble rx1, gdouble ry1, gdouble rx2, gdouble ry2)
{
    gdouble lenSqr = (lx2 - lx1) * (lx2 - lx1) + (ly2 - ly1) * (ly2 - ly1);
    gdouble cx1, cy1, cx2, cy2, a;
    gdouble d = 0.5 * thickness;
    gdouble len = sqrt(lenSqr);

    a = d * fabs(ly1 - ly2) / len;
    if( fmin(lx1, lx2) - fmax(rx1, rx2) > a
            || fmax(lx1, lx2) - fmin(rx1, rx2) < -a )
        return FALSE;
    a = d * fabs(lx1 - lx2) / len;
    if( fmin(ly1, ly2) - fmax(ry1, ry2) > a
            || fmax(ly1, ly2) - fmin(ry1, ry2) < -a )
        return FALSE;
    cx1 = rx1 * (ly2 - ly1);
    cx2 = rx2 * (ly2 - ly1);
    cy1 = ry1 * (lx1 - lx2);
    cy2 = ry2 * (lx1 - lx2);
    a = lx1 * ly2 - lx2 * ly1;
    if( fmin(cx1, cx2) + fmin(cy1, cy2) > a + d * len
            || fmax(cx1, cx2) + fmax(cy1, cy2) < a - d * len )
        return FALSE;
    cx1 = rx1 * (lx1 - lx2);
    cx2 = rx2 * (lx1 - lx2);
    cy1 = ry1 * (ly1 - ly2);
    cy2 = ry2 * (ly1 - ly2);
    a = lx1 * lx1 - lx2 * lx2 + ly1 * ly1 - ly2 * ly2;
    return 2.0 * (fmin(cx1, cx2) + fmin(cy1, cy2)) <= a + lenSqr
        && 2.0 * (fmax(cx1, cx2) + fmax(cy1, cy2)) >= a - lenSqr;
}

static void pathElemWithSectionHitTest(
        gdouble xPrev, gdouble yPrev, gdouble xBeg, gdouble yBeg,
        gdouble xEnd, gdouble yEnd, gdouble xNext, gdouble yNext,
        gdouble thickness,
        gdouble sx1, gdouble sy1, gdouble sx2, gdouble sy2,
        gboolean *isBegHit, gboolean *isEndHit)
{
    gdouble lsx1, lsx2, lsy1, lsy2, psx1, psx2, psy1, psy2;

    lsx1 = sx1;
    lsy1 = sy1;
    lsx2 = sx2;
    lsy2 = sy2;
    if( getSubsectionLyingOnLine(xBeg, yBeg, xEnd, yEnd,
                thickness, &lsx1, &lsy1, &lsx2, &lsy2))
    {
        psx1 = sx1;
        psy1 = sy1;
        psx2 = sx2;
        psy2 = sy2;
        if( ((xPrev == xBeg && yPrev == yBeg) ?
                getSubsectionLyingOnHalfplaneOrtogonal(xBeg, yBeg, xEnd, yEnd,
                    &psx1, &psy1, &psx2, &psy2)
                : getSubsectionLyingOnHalfplane(xPrev, yPrev, xBeg, yBeg,
                    thickness, &psx1, &psy1, &psx2, &psy2))
                && sectionsOverlap(lsx1, lsy1, lsx2, lsy2, psx1,
                    psy1, psx2, psy2))
            *isBegHit = TRUE;
        psx1 = sx1;
        psy1 = sy1;
        psx2 = sx2;
        psy2 = sy2;
        if( ((xNext == xEnd && yNext == yEnd) ?
                getSubsectionLyingOnHalfplaneOrtogonal(xEnd, yEnd, xBeg, yBeg,
                    &psx1, &psy1, &psx2, &psy2)
                : getSubsectionLyingOnHalfplane(xEnd, yEnd, xNext, yNext,
                    thickness, &psx1, &psy1, &psx2, &psy2))
                && sectionsOverlap(lsx1, lsy1, lsx2, lsy2, psx1,
                    psy1, psx2, psy2))
            *isEndHit = TRUE;
    }
} 

static gboolean pathElemHitTest(gdouble xPrev, gdouble yPrev, gdouble xBeg,
        gdouble yBeg, gdouble xEnd, gdouble yEnd, gdouble xNext, gdouble yNext,
        gdouble thickness, gdouble rx1, gdouble ry1, gdouble rx2, gdouble ry2)
{
    gboolean isBegHit = FALSE, isEndHit = FALSE;

    printf("pathElemHitTest: prev=(%.0f, %.0f), beg=(%.0f, %.0f), "
            "end=(%.0f, %.0f), next=(%.0f, %.0f)\n",
            xPrev, yPrev, xBeg, yBeg, xEnd, yEnd, xNext, yNext);
    printf("                 rect=((%.0f, %.0f), (%.0f, %.0f))\n",
            rx1, ry1, rx2, ry2);
    pathElemWithSectionHitTest(xPrev, yPrev, xBeg, yBeg, xEnd, yEnd, xNext,
            yNext, thickness, rx1, ry1, rx2, ry1, &isBegHit, &isEndHit);
    pathElemWithSectionHitTest(xPrev, yPrev, xBeg, yBeg, xEnd, yEnd, xNext,
            yNext, thickness, rx1, ry2, rx2, ry2, &isBegHit, &isEndHit);
    pathElemWithSectionHitTest(xPrev, yPrev, xBeg, yBeg, xEnd, yEnd, xNext,
            yNext, thickness, rx1, ry1, rx1, ry2, &isBegHit, &isEndHit);
    pathElemWithSectionHitTest(xPrev, yPrev, xBeg, yBeg, xEnd, yEnd, xNext,
            yNext, thickness, rx2, ry1, rx2, ry2, &isBegHit, &isEndHit);
    printf("isBegHit=%d, isEndHit%d\n\n", isBegHit, isEndHit);
    return isBegHit && isEndHit;
}

static gboolean ellipseHitTest(
        gdouble ex1, gdouble ey1, gdouble ex2, gdouble ey2, gdouble thickness,
        gdouble sx1, gdouble sx2, gdouble sy1, gdouble sy2)
{
    gdouble exmid = 0.5 * (ex1 + ex2);
    gdouble eymid = 0.5 * (ey1 + ey2);
    gdouble sxmin = fmin(sx1, sx2);
    gdouble sxmax = fmax(sx1, sx2);
    gdouble symin = fmin(sy1, sy2);
    gdouble symax = fmax(sy1, sy2);
    gdouble a = 0.5 * (G_SQRT2 * fabs(ex2 - ex1) + thickness);
    gdouble b = 0.5 * (G_SQRT2 * fabs(ey2 - ey1) + thickness);
    gdouble sxchk, sychk;

    if( sxmax < exmid ) {
        sxchk = sxmax;
    }else if( sxmin > exmid ) {
        sxchk = sxmin;
    }else
        sxchk = exmid;
    if( symax < eymid ) {
        sychk = symax;
    }else if( symin > eymid ) {
        sychk = symin;
    }else
        sychk = eymid;
    return b * b * (sxchk - exmid) * (sxchk - exmid)
            + a * a * (sychk - eymid) * (sychk - eymid) <= a * a * b * b;
}

gboolean hittest_line(gdouble lx1, gdouble ly1, gdouble lx2, gdouble ly2,
        gdouble thickness, gdouble rx1, gdouble ry1, gdouble rx2, gdouble ry2)
{
    /*
    return pathElemHitTest(lx1, ly1, lx1, ly1, lx2, ly2, lx2, ly2, thickness,
            rx1, ry1, rx2, ry2);
    */
    return lineHitTest(lx1, ly1, lx2, ly2, thickness, rx1, ry1, rx2, ry2);
}

gboolean hittest_arrow(gdouble lx1, gdouble ly1, gdouble lx2, gdouble ly2,
        gdouble thickness, gdouble rx1, gdouble ry1, gdouble rx2, gdouble ry2)
{
    return lineHitTest(lx1, ly1, lx2, ly2, thickness, rx1, ry1, rx2, ry2);
}

gboolean hittest_path(DrawPoint *pt, int ptCount,
        gdouble thickness, gdouble rx1, gdouble ry1, gdouble rx2, gdouble ry2)
{
    int i;
    gdouble xBeg = 0.0, yBeg = 0.0, xEnd, yEnd;

    for(i = 0; i < ptCount - 1; ++i) {
        xEnd = pt[i].x;
        yEnd = pt[i].y;
        if( lineHitTest(xBeg, yBeg, xEnd, yEnd, thickness, rx1, ry1, rx2, ry2))
            return TRUE;
        xBeg = xEnd;
        yBeg = yEnd;
    }
    xEnd = pt[ptCount - 1].x;
    yEnd = pt[ptCount - 1].y;
    return lineHitTest(xBeg, yBeg, xEnd, yEnd, thickness, rx1, ry1, rx2, ry2);
}

gboolean hittest_triangle(gdouble txBeg, gdouble tyBeg,
        gdouble txEnd, gdouble tyEnd,
        gdouble angle, gdouble round, gdouble thickness,
        gdouble rx1, gdouble ry1, gdouble rx2, gdouble ry2)
{
    gdouble d = round / sin(angle * G_PI / 360) - round;
    gdouble height = sqrt((txEnd - txBeg) * (txEnd - txBeg)
                + (tyEnd - tyBeg) * (tyEnd - tyBeg));
    gdouble c = tan(angle * G_PI / 360);
    gdouble txSub = (txEnd - txBeg) * d / height;
    gdouble tySub = (tyEnd - tyBeg) * d / height;
    txBeg -= txSub;
    tyBeg -= tySub;
    gdouble tx1 = txBeg;
    gdouble ty1 = tyBeg;
    gdouble tx2 = txEnd + (tyEnd - tyBeg) * c;
    gdouble ty2 = tyEnd - (txEnd - txBeg) * c;
    gdouble tx3 = txEnd - (tyEnd - tyBeg) * c;
    gdouble ty3 = tyEnd + (txEnd - txBeg) * c;
    return twoHalfplanesIntersectionHitTest(tx1, ty1, tx2, ty2, tx2, ty2,
                tx3, ty3, thickness, rx1, ry1, rx2, ry2)
        && twoHalfplanesIntersectionHitTest(tx2, ty2, tx3, ty3, tx3, ty3,
                tx1, ty1, thickness, rx1, ry1, rx2, ry2)
        && twoHalfplanesIntersectionHitTest(tx3, ty3, tx1, ty1, tx1, ty1,
                tx2, ty2, thickness, rx1, ry1, rx2, ry2);
}

gboolean hittest_rect(gdouble rtx1, gdouble rty1, gdouble rtx2, gdouble rty2,
        gdouble round, gdouble thickness,
        gdouble rx1, gdouble ry1, gdouble rx2, gdouble ry2)
{
    gdouble d = 0.5 * thickness;
    gdouble xBeg = fmin(rtx1, rtx2);
    gdouble xEnd = fmax(rtx1, rtx2);
    gdouble yBeg = fmin(rty1, rty2);
    gdouble yEnd = fmax(rty1, rty2);

    round = fmin(round, 0.5 * G_SQRT2 * fmin(xEnd - xBeg, yEnd - yBeg));
    xBeg -= (1 - 0.5 * G_SQRT2) * round;
    yBeg -= (1 - 0.5 * G_SQRT2) * round;
    xEnd += (1 - 0.5 * G_SQRT2) * round;
    yEnd += (1 - 0.5 * G_SQRT2) * round;
    return fmin(rx1, rx2) <= xEnd + d
        && fmin(ry1, ry2) <= yEnd + d
        && fmax(rx1, rx2) >= xBeg - d
        && fmax(ry1, ry2) >= yBeg - d;
}

gboolean hittest_ellipse(
        gdouble ex1, gdouble ey1, gdouble ex2, gdouble ey2,
        gdouble thickness, gdouble rx1, gdouble ry1, gdouble rx2, gdouble ry2)
{
    return ellipseHitTest(ex1, ey1, ex2, ey2, thickness, rx1, rx2, ry1, ry2);
}

