#ifndef SHAPEDRAWING_H
#define SHAPEDRAWING_H


void sd_pathPoint(cairo_t *cr, gdouble x, gdouble y);

void sd_pathLine(cairo_t *cr, gdouble xleft, gdouble yTop, gdouble xRight,
        gdouble yBottom, gdouble angle, gdouble round);

/* "Proportion" parameter is a ratio of arrow head width to line width
 */
void sd_pathArrow(cairo_t *cr, gdouble shapeXRef, gdouble shapeYRef,
        gdouble shapeXEnd, gdouble shapeYEnd, gdouble thickness,
        gdouble proportion, gdouble angle);

void sd_pathTriangle(cairo_t *cr, gdouble xBeg, gdouble yBeg,
        gdouble xEnd, gdouble yEnd, gdouble angle, gdouble round);

void sd_pathRect(cairo_t *cr, gdouble xBeg, gdouble yBeg,
        gdouble xEnd, gdouble yEnd, gdouble round);

void sd_pathOval(cairo_t *cr, gdouble xBeg, gdouble yBeg,
        gdouble xEnd, gdouble yEnd);

#endif /* SHAPEDRAWING_H */
