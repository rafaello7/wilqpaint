#include <gtk/gtk.h>
#include "drawimage.h"
#include <string.h>
#include <math.h>


enum {
    UNDO_MAX = 1000,
    STIDX_NOOP          = -1,
    STIDX_NEWSHAPE      = -2,
    STIDX_DELSHAPE      = -3,
    STIDX_SETBACKGROUND = -4,
    STIDX_SCALE         = -5,
    STIDX_MOVE          = -6
};

typedef struct {
    guint id;                   /* the state id */
    gint imgWidth, imgHeight;
    gdouble imgXRef, imgYRef;
    GdkRGBA imgBgColor;
    cairo_surface_t *baseImage;
    Shape **shapes;
    int shapeCount;
} DrawImageState;

struct DrawImage {
    DrawImageState states[UNDO_MAX];
    gint stateFirst, stateCur, stateLast;
    gint curShapeIdx; /* STIDX_* or shape index */
    gboolean isCurShapeModified;
    guint nextStateId;
    guint savedStateId;
};

DrawImage *di_new(gint imgWidth, gint imgHeight)
{
    DrawImage *di = g_malloc(sizeof(DrawImage));
    di->states[0].id = 0;
    di->states[0].imgWidth = imgWidth;
    di->states[0].imgHeight = imgHeight;
    di->states[0].imgXRef = 0.0;
    di->states[0].imgYRef = 0.0;
    di->states[0].baseImage = NULL;
    di->states[0].imgBgColor.red = 1.0;
    di->states[0].imgBgColor.green = 1.0;
    di->states[0].imgBgColor.blue = 1.0;
    di->states[0].imgBgColor.alpha = 1.0;
    di->states[0].shapes = NULL;
    di->states[0].shapeCount = 0;
    di->stateFirst = 0;
    di->stateCur = 0;
    di->stateLast = 0;
    di->curShapeIdx = STIDX_NOOP;
    di->isCurShapeModified = FALSE;
    di->nextStateId = 1;
    di->savedStateId = 0;
}

static void freeState(DrawImageState *state)
{
    int i;

    if( state->baseImage != NULL )
        cairo_surface_destroy(state->baseImage);
    for(i = 0; i < state->shapeCount; ++i)
        shape_unref(state->shapes[i]);
    g_free(state->shapes);
}

static void freeStates(DrawImageState *states, int first, int last)
{
    while( first != last )
        freeState(states + first++);
    freeState(states + first);
}

static DrawImageState *getStateForModify(DrawImage *di, gint shapeIdx)
{
    DrawImageState *prev, *cur;
    int i;

    cur = di->states + di->stateCur;
    g_assert_cmpint(di->curShapeIdx, <, cur->shapeCount);
    if( shapeIdx != di->curShapeIdx || ! di->isCurShapeModified ) {
        prev = cur;
        i = di->stateCur;
        if( ++di->stateCur == UNDO_MAX )
            di->stateCur = 0;
        if( i != di->stateLast )
            freeStates(di->states, di->stateCur, di->stateLast);
        di->stateLast = di->stateCur;
        if( di->stateCur == di->stateFirst ) {
            freeState(di->states + di->stateFirst);
            if( ++di->stateFirst == UNDO_MAX )
                di->stateFirst = 0;
        }
        cur = di->states + di->stateCur;
        cur->id = di->nextStateId++;
        cur->imgWidth = prev->imgWidth;
        cur->imgHeight = prev->imgHeight;
        cur->imgXRef = prev->imgXRef;
        cur->imgYRef = prev->imgYRef;
        cur->imgBgColor = prev->imgBgColor;
        if( prev->baseImage != NULL )
            cur->baseImage = cairo_surface_reference(prev->baseImage);
        else
            cur->baseImage = NULL;
        cur->shapes = g_malloc(prev->shapeCount * sizeof(Shape*));
        for(i = 0; i < prev->shapeCount; ++i) {
            cur->shapes[i] = prev->shapes[i];
            shape_ref(cur->shapes[i]);
        }
        cur->shapeCount = prev->shapeCount;
        if( shapeIdx >= 0 )
            shape_replaceDup(cur->shapes + shapeIdx);
        di->curShapeIdx = shapeIdx;
        di->isCurShapeModified = TRUE;
    }
    return cur;
}

DrawImage *di_open(const char *fileName)
{
    DrawImage *di = NULL;

    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(fileName, NULL);
    if( pixbuf != NULL ) {
        di = di_new(gdk_pixbuf_get_width(pixbuf),
                gdk_pixbuf_get_height(pixbuf));
        di->states[0].baseImage
            = gdk_cairo_surface_create_from_pixbuf(pixbuf, 1, NULL);
        g_object_unref(G_OBJECT(pixbuf));
    }
    return di;
}

gint di_getWidth(const DrawImage *di)
{
    return di->states[di->stateCur].imgWidth;
}

gint di_getHeight(const DrawImage *di)
{
    return di->states[di->stateCur].imgHeight;
}

gdouble di_getXRef(const DrawImage *di)
{
    return di->states[di->stateCur].imgXRef;
}

gdouble di_getYRef(const DrawImage *di)
{
    return di->states[di->stateCur].imgYRef;
}

void di_setBackgroundColor(DrawImage *di, const GdkRGBA *color)
{
    DrawImageState *state = getStateForModify(di, STIDX_SETBACKGROUND);
    state->imgBgColor = *color;
}

void di_getBackgroundColor(const DrawImage *di, GdkRGBA *color)
{
    *color = di->states[di->stateCur].imgBgColor;
}

void di_addShape(DrawImage *di, ShapeType shapeType,
        gdouble xRef, gdouble yRef, const ShapeParams *shapeParams)
{
    Shape *shape;
    DrawImageState *state = getStateForModify(di, STIDX_NEWSHAPE);

    shape = shape_new(shapeType, xRef - state->imgXRef, yRef - state->imgYRef,
            shapeParams);
    di->curShapeIdx = state->shapeCount;
    state->shapes = g_realloc(state->shapes,
            ++state->shapeCount * sizeof(Shape*));
    state->shapes[di->curShapeIdx] = shape;
}

gboolean di_isCurShapeSet(const DrawImage *di)
{
    return di->curShapeIdx >= 0;
}

static Shape *curShape(const DrawImage *di)
{
    const DrawImageState *state = di->states + di->stateCur;
    g_assert(di->curShapeIdx < state->shapeCount);
    return state->shapes[di->curShapeIdx];
}

void di_curShapeAddPoint(DrawImage *di, gdouble x, gdouble y)
{
    if( di->curShapeIdx >= 0 ) {
        DrawImageState *state = getStateForModify(di, di->curShapeIdx);
        shape_addPoint(state->shapes[di->curShapeIdx],
                x - state->imgXRef, y - state->imgYRef);
    }
}

gdouble di_getCurShapeXRef(const DrawImage *di)
{
    const DrawImageState *state = di->states + di->stateCur;
    return di->curShapeIdx >= 0 ?
        shape_getXRef(curShape(di)) + state->imgXRef : -1.0;
}

gdouble di_getCurShapeYRef(const DrawImage *di)
{
    const DrawImageState *state = di->states + di->stateCur;
    return di->curShapeIdx >= 0 ?
        shape_getYRef(curShape(di)) + state->imgYRef : -1.0;
}

ShapeType di_getCurShapeType(const DrawImage *di)
{
    g_assert_cmpint(di->curShapeIdx, >=, 0);
    return shape_getType(curShape(di));
}

void di_getCurShapeParams(const DrawImage *di, ShapeParams *shapeParams)
{
    if( di->curShapeIdx >= 0 )
        shape_getParams(curShape(di), shapeParams);
}

void di_setCurShapeParams(DrawImage *di, const ShapeParams *shapeParams)
{
    if( di->curShapeIdx >= 0 ) {
        DrawImageState *state = getStateForModify(di, di->curShapeIdx);
        shape_setParams(state->shapes[di->curShapeIdx], shapeParams);
    }
}

void di_curShapeMoveTo(DrawImage *di, gdouble x, gdouble y)
{
    if( di->curShapeIdx >= 0 ) {
        DrawImageState *state = getStateForModify(di, di->curShapeIdx);
        shape_moveTo(state->shapes[di->curShapeIdx],
                x - state->imgXRef, y - state->imgYRef);
    }
}

void di_curShapeRemove(DrawImage *di)
{
    int shapeIdx;

    if( di->curShapeIdx >= 0 ) {
        shapeIdx = di->curShapeIdx;
        DrawImageState *state = getStateForModify(di, STIDX_DELSHAPE);
        shape_unref(state->shapes[shapeIdx]);
        while( ++shapeIdx < state->shapeCount )
            state->shapes[shapeIdx-1] = state->shapes[shapeIdx];
        --state->shapeCount;
        di->curShapeIdx = STIDX_NOOP;
    }
}

void di_undo(DrawImage *di)
{
    if( di->stateCur != di->stateFirst ) {
        if( di->stateCur == 0 )
            di->stateCur = UNDO_MAX - 1;
        else
            --di->stateCur;
        di->curShapeIdx = STIDX_NOOP;
    }
}

void di_redo(DrawImage *di)
{
    if( di->stateCur != di->stateLast ) {
        if( ++di->stateCur == UNDO_MAX )
            di->stateCur = 0;
        di->curShapeIdx = STIDX_NOOP;
    }
}

gboolean di_curShapeFromPoint(DrawImage *di, gdouble x, gdouble y)
{
    DrawImageState *state = di->states + di->stateCur;
    int i;

    for(i = state->shapeCount - 1; i >= 0
            && !shape_hitTest(state->shapes[i], x - state->imgXRef,
                y - state->imgYRef); --i)
    {
    }
    di->curShapeIdx = i;
    di->isCurShapeModified = FALSE;
    return i >= 0;
}

void di_scale(DrawImage *di, gdouble factor)
{
    int i;
    DrawImageState *state = getStateForModify(di, STIDX_SCALE);

    for(i = 0; i < state->shapeCount; ++i) {
        Shape *shape = shape_replaceDup(state->shapes + i);
        shape_scale(shape, factor);
    }
    state->imgXRef *= factor;
    state->imgYRef *= factor;
    state->imgWidth = round(state->imgWidth * factor);
    state->imgHeight = round(state->imgHeight * factor);
    if( state->baseImage != NULL ) {
        cairo_surface_t *newImage = cairo_image_surface_create(
                CAIRO_FORMAT_ARGB32, state->imgWidth, state->imgHeight);
        cairo_t *cr = cairo_create(newImage);
        cairo_scale(cr, factor, factor);
        cairo_set_source_surface(cr, state->baseImage, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);
        cairo_surface_destroy(state->baseImage);
        state->baseImage = newImage;
    }
}

void di_moveTo(DrawImage *di, gdouble imgXRef, gdouble imgYRef)
{
    DrawImageState *state = getStateForModify(di, STIDX_MOVE);
    state->imgXRef = imgXRef;
    state->imgYRef = imgYRef;
}

void di_setSize(DrawImage *di, gint imgWidth, gint imgHeight,
        gdouble translateXfactor, gdouble translateYfactor)
{
    DrawImageState *state = getStateForModify(di, STIDX_MOVE);
    state->imgXRef += translateXfactor * (imgWidth - state->imgWidth);
    state->imgYRef += translateYfactor * (imgHeight - state->imgHeight);
    state->imgWidth = imgWidth;
    state->imgHeight = imgHeight;

}

void di_draw(const DrawImage *di, cairo_t *cr)
{
    cairo_matrix_t matrix;
    double xBeg, yBeg, xEnd, yEnd, angleTan, round, lineLen;
    const DrawImageState *state = di->states + di->stateCur;

    gdk_cairo_set_source_rgba(cr, &state->imgBgColor);
    cairo_paint(cr);
    if( state->imgXRef != 0.0 || state->imgYRef != 0.0 ) {
        cairo_save(cr);
        cairo_translate(cr, state->imgXRef, state->imgYRef);
    }
    if( state->baseImage != NULL ) {
        cairo_set_source_surface(cr, state->baseImage, 0, 0);
        cairo_paint(cr);
    }
    for(int i = 0; i < state->shapeCount; ++i)
        shape_draw(state->shapes[i], cr);
    if( state->imgXRef != 0.0 || state->imgYRef != 0.0 )
        cairo_restore(cr);
}

static const char *getFileFormatByExt(const char *fname)
{
    static char *res;
    GSList *formats, *item;
    const char *ext;
    char *basename;
    int i;

    g_free(res);
    res = NULL;
    basename = g_path_get_basename(fname);
    ext = strrchr(basename, '.');
    if( ext != NULL ) {
        ++ext;
        formats = gdk_pixbuf_get_formats();
        for(item = formats; item != NULL; item = g_slist_next(item)) {
            GdkPixbufFormat *fmt = item->data;
            if( ! gdk_pixbuf_format_is_writable(fmt) )
                continue;
            gchar **extensions = gdk_pixbuf_format_get_extensions(fmt);
            for(i = 0; extensions[i] && res == NULL; ++i) {
                if( ! strcmp(ext, extensions[i]) )
                    res = gdk_pixbuf_format_get_name(fmt);
            }
            g_strfreev(extensions);
        }
        g_slist_free(formats);
    }
    g_free(basename);
    return res == NULL ? "png" : res;
}

void di_save(DrawImage *di, const char *fileName)
{
    const DrawImageState *state = di->states + di->stateCur;
    cairo_surface_t *paintImage = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32, state->imgWidth, state->imgHeight);
    cairo_t *cr = cairo_create(paintImage);
    di_draw(di, cr);
    cairo_destroy(cr);
    GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(paintImage,
            0, 0, state->imgWidth, state->imgHeight);
    cairo_surface_destroy(paintImage);
    gdk_pixbuf_save(pixbuf, fileName, getFileFormatByExt(fileName), NULL, NULL);
    g_object_unref(G_OBJECT(pixbuf));
    di->curShapeIdx = STIDX_NOOP;
    di->savedStateId = state->id;
}

gboolean di_isModified(const DrawImage *di)
{
    const DrawImageState *state = di->states + di->stateCur;

    return state->id != di->savedStateId;
}

void di_free(DrawImage *di)
{
    freeStates(di->states, di->stateFirst, di->stateLast);
    g_free(di);
}

