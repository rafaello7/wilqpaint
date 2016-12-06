#ifndef HITTEST_H
#define HITTEST_H

typedef struct {
    gdouble x, y;
} DrawPoint;

gboolean hittest_path(DrawPoint *pt, int ptCount,
        gdouble thickness, gdouble rx1, gdouble ry1, gdouble rx2, gdouble ry2);

gboolean hittest_line(gdouble lx1, gdouble ly1, gdouble lx2, gdouble ly2,
        gdouble thickness, gdouble rx1, gdouble ry1, gdouble rx2, gdouble ry2);

gboolean hittest_arrow(gdouble lx1, gdouble ly1, gdouble lx2, gdouble ly2,
        gdouble thickness, gdouble rx1, gdouble ry1, gdouble rx2, gdouble ry2);

gboolean hittest_triangle(gdouble tx1, gdouble ty1, gdouble tx2, gdouble ty2,
        gdouble tx3, gdouble ty3, gdouble thickness,
        gdouble rx1, gdouble ry1, gdouble rx2, gdouble ry2);

gboolean hittest_rect(gdouble rtx1, gdouble rty1, gdouble rtx2, gdouble rty2,
        gdouble thickness, gdouble rx1, gdouble ry1, gdouble rx2, gdouble ry2);

gboolean hittest_ellipse(
        gdouble ex1, gdouble ey1, gdouble ex2, gdouble ey2,
        gdouble thickness, gdouble rx1, gdouble ry1, gdouble rx2, gdouble ry2);

#endif /* HITTEST_H */
