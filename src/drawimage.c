#include <gtk/gtk.h>
#include "drawimage.h"
#include <string.h>
#include <math.h>
#include "wlqpersistence.h"


enum {
    UNDO_MAX = 1000,
};

enum StateModification {
    SM_NEW,                 /* new image */
    SM_SHAPE_LAYOUT_NEW_BEG,/* add a new shape and begin layout */
    SM_SHAPE_LAYOUT_NEW,    /* a new shape layout */
    SM_SHAPE_LAYOUT,        /* change layout of current shape */
    SM_SHAPE_ZORDER,        /* raise/bring down shape */
    SM_SHAPESIDE_MARK,      /* shape side is selected by mouse press
                             * goes to SM_SHAPE_LAYOUT when mouse is dragged.
                             * The shape is also (the only) selection member */
    SM_SELECTION_MARK,      /* select shapes */
    SM_SEL_DRAG,            /* move selected shapes (mouse drag) */
    SM_SEL_PARAM,           /* modify parameters of selected shapes */
    SM_SEL_DELETE,          /* delete selected shapes */
    SM_IMAGE_BACKGROUND,    /* set image background */
    SM_IMAGE_SCALE,         /* scale image */
    SM_IMAGE_SIZE,          /* change image size and position */
    SM_IMAGE_THRESHOLD,
    SM_UNDO_REDO            /* undo/redo operation */
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
    enum StateModification curStateModification;
    gint curShapeIdx;               /* current shape, -1 for none */
    enum ShapeCorner dragShapeCorner;
    guint nextStateId;
    guint savedStateId;
    GHashTable *selection;
    GHashTable *addedByRectSel;
    gdouble selXRef, selYRef;
    cairo_surface_t *preview;
};

DrawImage *di_new(gint imgWidth, gint imgHeight, const GdkPixbuf *baseImage)
{
    DrawImage *di = g_malloc(sizeof(DrawImage));
    di->states[0].id = 0;
    di->states[0].imgWidth = imgWidth;
    di->states[0].imgHeight = imgHeight;
    di->states[0].imgXRef = 0.0;
    di->states[0].imgYRef = 0.0;
    if( baseImage != NULL ) {
        di->states[0].baseImage = cairo_image_surface_create(
                CAIRO_FORMAT_ARGB32, gdk_pixbuf_get_width(baseImage),
                gdk_pixbuf_get_height(baseImage));
        cairo_t *cr = cairo_create(di->states[0].baseImage);
        gdk_cairo_set_source_pixbuf(cr, baseImage, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);
    }else
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
    di->curStateModification = SM_NEW;
    di->curShapeIdx = -1;
    di->dragShapeCorner = SC_NONE;
    di->nextStateId = 1;
    di->savedStateId = 0;
    di->selection = g_hash_table_new(NULL, NULL);
    di->addedByRectSel = g_hash_table_new(NULL, NULL);
    di->selXRef = 0;
    di->selYRef = 0;
    di->preview = NULL;
    return di;
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

static DrawImageState *getStateForModify(DrawImage *di,
        enum StateModification smod)
{
    DrawImageState *prev, *cur;
    int i;

    cur = di->states + di->stateCur;
    g_assert_cmpint(di->curShapeIdx, <, cur->shapeCount);
    if( smod != di->curStateModification ) {
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
        gboolean isModSel = smod == SM_SEL_DRAG || smod == SM_SEL_PARAM
            || smod == SM_SEL_DELETE;
        for(i = 0; i < prev->shapeCount; ++i) {
            cur->shapes[i] = prev->shapes[i];
            shape_ref(cur->shapes[i]);
            if( isModSel && g_hash_table_contains(di->selection,
                        GINT_TO_POINTER(i)) )
                shape_replaceDup(cur->shapes + i);
        }
        if( smod == SM_SHAPE_LAYOUT )
            shape_replaceDup(cur->shapes + di->curShapeIdx);
        cur->shapeCount = prev->shapeCount;
        di->curStateModification = smod;
    }
    return cur;
}

DrawImage *di_openWLQ(const char *fileName, gchar **errLoc,
        gboolean *isNoEntErr)
{
    static cairo_user_data_key_t dataKey;
    WlqInFile *inFile;
    unsigned imgWidth, imgHeight, imgStride, shapeCount;
    DrawImage *di;
    DrawImageState *state;

    if( (inFile = wlq_openIn(fileName, errLoc, isNoEntErr)) == NULL )
        return NULL;
    if( wlq_readU32(inFile, &imgWidth, errLoc)
            && wlq_readU32(inFile, &imgHeight, errLoc) )
    {
        di = di_new(imgWidth, imgHeight, NULL);
        state = di->states;
        gboolean isOK = wlq_readCoordinate(inFile, &state->imgXRef, errLoc)
                && wlq_readCoordinate(inFile, &state->imgYRef, errLoc)
                && wlq_readRGBA(inFile, &state->imgBgColor, errLoc)
                && wlq_readU32(inFile, &imgWidth, errLoc)
                && wlq_readU32(inFile, &imgHeight, errLoc)
                && wlq_readU32(inFile, &imgStride, errLoc);
        if( isOK && imgWidth && imgHeight && imgStride ) {
            char *data = g_try_malloc(imgHeight * imgStride);
            if( data != NULL ) {
                isOK = wlq_read(inFile, data, imgHeight * imgStride, errLoc);
                if( isOK ) {
                    state->baseImage = cairo_image_surface_create_for_data(
                            data, CAIRO_FORMAT_ARGB32, imgWidth, imgHeight,
                            imgStride);
                    cairo_surface_set_user_data(state->baseImage, &dataKey,
                            data, g_free);
                }else
                    g_free(data);
            }else{
                *errLoc = g_strdup_printf("%s: not enough memory for"
                        " base image %ux%u", fileName, imgWidth, imgHeight);
                isOK = FALSE;
            }
        }
        if( isOK ) {
            isOK = wlq_readU32(inFile, &shapeCount, errLoc);
            if( isOK && shapeCount ) {
                state->shapes = g_try_malloc(shapeCount * sizeof(Shape*));
                if( state->shapes != NULL ) {
                    while( state->shapeCount < shapeCount ) {
                        if( (state->shapes[state->shapeCount]
                                = shape_readFromFile(inFile, errLoc)) == NULL)
                        {
                            isOK = FALSE;
                            break;
                        }
                        ++state->shapeCount;
                    }
                }else{
                    *errLoc = g_strdup_printf("%s: not enough memory to load "
                            "%u shapes from file", fileName, shapeCount);
                    isOK = FALSE;
                }
            }
        }
        if( ! isOK ) {
            di_free(di);
            di = NULL;
        }
    }
    wlq_closeIn(inFile);
    *isNoEntErr = FALSE;
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

gboolean di_hasBaseImage(const DrawImage *di)
{
    return di->states[di->stateCur].baseImage != NULL;
}

void di_getBackgroundColor(const DrawImage *di, GdkRGBA *color)
{
    *color = di->states[di->stateCur].imgBgColor;
}

void di_setBackgroundColor(DrawImage *di, const GdkRGBA *color)
{
    DrawImageState *state = getStateForModify(di, SM_IMAGE_BACKGROUND);
    state->imgBgColor = *color;
}

void di_addShape(DrawImage *di, ShapeType shapeType,
        gdouble xRef, gdouble yRef, const ShapeParams *shapeParams,
        gboolean addBottom)
{
    Shape *shape;
    DrawImageState *state;

    di->curStateModification = SM_SHAPE_LAYOUT_NEW_BEG;
    state = getStateForModify(di, SM_SHAPE_LAYOUT_NEW);
    shape = shape_new(shapeType, xRef - state->imgXRef, yRef - state->imgYRef,
            shapeParams);
    di->curShapeIdx = state->shapeCount;
    state->shapes = g_realloc(state->shapes,
            ++state->shapeCount * sizeof(Shape*));
    if( addBottom ) {
        while( di->curShapeIdx > 0 ) {
            state->shapes[di->curShapeIdx] = state->shapes[di->curShapeIdx-1];
            --di->curShapeIdx;
        }
    }
    state->shapes[di->curShapeIdx] = shape;
    di->selXRef = xRef;
    di->selYRef = yRef;
    g_hash_table_remove_all(di->selection);
}

gboolean di_isSelectionEmpty(const DrawImage *di)
{
    return g_hash_table_size(di->selection) == 0;
}

static Shape *curShape(const DrawImage *di)
{
    const DrawImageState *state = di->states + di->stateCur;
    g_assert(di->curShapeIdx < state->shapeCount);
    return state->shapes[di->curShapeIdx];
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

void di_curShapeRaise(DrawImage *di)
{
    Shape *shape;

    DrawImageState *state = di->states + di->stateCur;
    if( di->curShapeIdx >= 0 && di->curShapeIdx + 1 < state->shapeCount ) {
        state = getStateForModify(di, SM_SHAPE_ZORDER);
        g_hash_table_remove(di->selection, GINT_TO_POINTER(di->curShapeIdx));
        shape = state->shapes[di->curShapeIdx];
        while( di->curShapeIdx + 1 < state->shapeCount ) {
            state->shapes[di->curShapeIdx] = state->shapes[di->curShapeIdx+1];
            ++di->curShapeIdx;
        }
        state->shapes[di->curShapeIdx] = shape;
        g_hash_table_add(di->selection, GINT_TO_POINTER(di->curShapeIdx));
    }
}

void di_curShapeSink(DrawImage *di)
{
    Shape *shape;

    DrawImageState *state = di->states + di->stateCur;
    if( di->curShapeIdx > 0 ) {
        state = getStateForModify(di, SM_SHAPE_ZORDER);
        g_hash_table_remove(di->selection, GINT_TO_POINTER(di->curShapeIdx));
        shape = state->shapes[di->curShapeIdx];
        while( di->curShapeIdx > 0 ) {
            state->shapes[di->curShapeIdx] = state->shapes[di->curShapeIdx-1];
            --di->curShapeIdx;
        }
        state->shapes[di->curShapeIdx] = shape;
        g_hash_table_add(di->selection, GINT_TO_POINTER(di->curShapeIdx));
    }
}

void di_undo(DrawImage *di)
{
    if( di->stateCur != di->stateFirst ) {
        if( di->stateCur == 0 )
            di->stateCur = UNDO_MAX - 1;
        else
            --di->stateCur;
        di->curShapeIdx = -1;
        g_hash_table_remove_all(di->selection);
        di->curStateModification = SM_UNDO_REDO;
    }
}

void di_redo(DrawImage *di)
{
    if( di->stateCur != di->stateLast ) {
        if( ++di->stateCur == UNDO_MAX )
            di->stateCur = 0;
        di->curShapeIdx = -1;
        g_hash_table_remove_all(di->selection);
        di->curStateModification = SM_UNDO_REDO;
    }
}

gboolean di_curShapeFromPoint(DrawImage *di, gdouble x, gdouble y,
        gdouble zoom, gboolean extendSel)
{
    DrawImageState *state = di->states + di->stateCur;
    int shapeIdx;
    gpointer idxAsPtr;
    enum ShapeCorner corner;

    g_hash_table_remove_all(di->addedByRectSel);
    if( ! extendSel )
        g_hash_table_remove_all(di->selection);
    di->curShapeIdx = -1;
    di->curStateModification = SM_SELECTION_MARK;
    di->selXRef = x;
    di->selYRef = y;
    shapeIdx = state->shapeCount - 1;
    while( shapeIdx >= 0 && di->curShapeIdx == -1 ) {
        if( (corner = shape_cornerHitTest(state->shapes[shapeIdx],
                x - state->imgXRef, y - state->imgYRef, zoom)) != SC_NONE )
        {
            di->curShapeIdx = shapeIdx;
            idxAsPtr = GINT_TO_POINTER(shapeIdx);
            if( ! g_hash_table_contains(di->selection, idxAsPtr) )
                g_hash_table_add(di->selection, idxAsPtr);
            if( ! extendSel ) {
                di->curStateModification = SM_SHAPESIDE_MARK;
                di->dragShapeCorner = corner;
            }
        }else if( shape_hitTest(state->shapes[shapeIdx],
                x - state->imgXRef, y - state->imgYRef,
                x - state->imgXRef, y - state->imgYRef) )
        {
            di->curShapeIdx = shapeIdx;
            idxAsPtr = GINT_TO_POINTER(shapeIdx);
            if( ! g_hash_table_contains(di->selection, idxAsPtr) )
                g_hash_table_add(di->selection, idxAsPtr);
        }
        --shapeIdx;
    }
    return di->curShapeIdx != -1;
}

void di_selectionFromPoint(DrawImage *di, gdouble x, gdouble y,
        gboolean extend)
{
    DrawImageState *state = di->states + di->stateCur;
    int shapeIdx;
    gpointer idxAsPtr;

    g_hash_table_remove_all(di->addedByRectSel);
    if( ! extend )
        g_hash_table_remove_all(di->selection);
    di->curShapeIdx = -1;
    di->curStateModification = SM_SELECTION_MARK;
    di->selXRef = x;
    di->selYRef = y;
    for( shapeIdx = state->shapeCount - 1; shapeIdx >= 0; --shapeIdx ) {
        idxAsPtr = GINT_TO_POINTER(shapeIdx);
        if( ! g_hash_table_contains(di->selection, idxAsPtr)
            && shape_hitTest(state->shapes[shapeIdx],
                x - state->imgXRef, y - state->imgYRef,
                x - state->imgXRef, y - state->imgYRef) )
        {
            g_hash_table_add(di->addedByRectSel, idxAsPtr);
            g_hash_table_add(di->selection, idxAsPtr);
        }
    }
}

void di_selectionFromRect(DrawImage *di, gdouble x, gdouble y)
{
    DrawImageState *state = di->states + di->stateCur;
    int shapeIdx;
    GHashTableIter iter;
    gpointer idxAsPtr;

    g_assert_cmpint(di->curStateModification, ==, SM_SELECTION_MARK);
    g_hash_table_iter_init(&iter, di->addedByRectSel);
    while( g_hash_table_iter_next(&iter, &idxAsPtr, NULL) )
        g_hash_table_remove(di->selection, idxAsPtr);
    g_hash_table_remove_all(di->addedByRectSel);
    for(shapeIdx = 0; shapeIdx < state->shapeCount; ++shapeIdx) {
        idxAsPtr = GINT_TO_POINTER(shapeIdx);
        if( ! g_hash_table_contains(di->selection, idxAsPtr)
            && shape_hitTest(state->shapes[shapeIdx],
                di->selXRef - state->imgXRef, di->selYRef - state->imgYRef,
                x - state->imgXRef, y - state->imgYRef) )
        {
            g_hash_table_add(di->addedByRectSel, idxAsPtr);
            g_hash_table_add(di->selection, idxAsPtr);
        }
    }
}

void di_setSelectionParam(DrawImage *di, enum ShapeParam shapeParam,
        const ShapeParams *shapeParams)
{
    GHashTableIter iter;
    gpointer key;

    if( g_hash_table_size(di->selection) > 0 ) {
        DrawImageState *state = getStateForModify(di, SM_SEL_PARAM);
        g_hash_table_iter_init(&iter, di->selection);
        while( g_hash_table_iter_next(&iter, &key, NULL) ) {
            gint shapeIdx = GPOINTER_TO_INT(key);
            shape_setParam(state->shapes[shapeIdx], shapeParam, shapeParams);
        }
    }
}

void di_selectionDragTo(DrawImage *di, gdouble x, gdouble y, gboolean even)
{
    GHashTableIter iter;
    gpointer key;
    DrawImageState *state, *stPrev;
    gdouble mvX, mvY;

    switch( di->curStateModification ) {
    case SM_SHAPE_LAYOUT_NEW:
        state = getStateForModify(di, SM_SHAPE_LAYOUT_NEW);
        shape_layoutNew(state->shapes[di->curShapeIdx],
                x - state->imgXRef, y - state->imgYRef, even);
        break;
    case SM_SHAPESIDE_MARK:
    case SM_SHAPE_LAYOUT:
        state = getStateForModify(di, SM_SHAPE_LAYOUT);
        stPrev = di->states + (di->stateCur == 0 ? UNDO_MAX : di->stateCur) - 1;
        shape_layout(state->shapes[di->curShapeIdx],
                stPrev->shapes[di->curShapeIdx],
                x - di->selXRef, y - di->selYRef, di->dragShapeCorner, even);
        break;
    case SM_SELECTION_MARK:
    case SM_SEL_DRAG:
        if( g_hash_table_size(di->selection) > 0 ) {
            state = getStateForModify(di, SM_SEL_DRAG);
            stPrev = di->states
                    + (di->stateCur == 0 ? UNDO_MAX : di->stateCur) - 1;
            g_hash_table_iter_init(&iter, di->selection);
            mvX = x - di->selXRef;
            mvY = y - di->selYRef;
            if( even ) {
                if( fabs(mvX) >= fabs(mvY) )
                    mvY = 0;
                else
                    mvX = 0;
            }
            while( g_hash_table_iter_next(&iter, &key, NULL) ) {
                gint shapeIdx = GPOINTER_TO_INT(key);
                shape_move(state->shapes[shapeIdx],
                        stPrev->shapes[shapeIdx], mvX, mvY);
            }
        }
        break;
    default:
        g_warning("di_selectionDragTo: unexpected state=%d",
                di->curStateModification);
        break;
    }
}

gboolean di_selectionDelete(DrawImage *di)
{

    if( g_hash_table_size(di->selection) >= 0 ) {
        int src, dest = 0;
        DrawImageState *state = getStateForModify(di, SM_SEL_DELETE);
        for(src = 0; src < state->shapeCount; ++src) {
            if( g_hash_table_contains(di->selection, GINT_TO_POINTER(src)) ) {
                shape_unref(state->shapes[src]);
            }else{
                if( dest != src )
                    state->shapes[dest] = state->shapes[src];
                ++dest;
            }
        }
        state->shapeCount = dest;
        di->curShapeIdx = -1;
        g_hash_table_remove_all(di->selection);
        return TRUE;
    }
    return FALSE;
}

gboolean di_selectionSetEmpty(DrawImage *di)
{
    gboolean wasEmpty = g_hash_table_size(di->selection) == 0;

    if( ! wasEmpty )
        g_hash_table_remove_all(di->selection);
    di->curShapeIdx = -1;
    return ! wasEmpty;
}

void di_scale(DrawImage *di, gdouble factor)
{
    int i;
    DrawImageState *state = getStateForModify(di, SM_IMAGE_SCALE);

    for(i = 0; i < state->shapeCount; ++i) {
        Shape *shape = shape_replaceDup(state->shapes + i);
        shape_scale(shape, factor);
    }
    state->imgXRef *= factor;
    state->imgYRef *= factor;
    state->imgWidth = round(state->imgWidth * factor);
    state->imgHeight = round(state->imgHeight * factor);
    if( state->baseImage != NULL ) {
        gint imgWidth = fmax(round(cairo_image_surface_get_width(
                        state->baseImage) * factor), 1);
        gint imgHeight = fmax(round(cairo_image_surface_get_height(
                        state->baseImage) * factor), 1);
        cairo_surface_t *newImage = cairo_image_surface_create(
                CAIRO_FORMAT_ARGB32, imgWidth, imgHeight);
        cairo_t *cr = cairo_create(newImage);
        cairo_scale(cr, factor, factor);
        cairo_set_source_surface(cr, state->baseImage, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);
        cairo_surface_destroy(state->baseImage);
        state->baseImage = newImage;
    }
    g_hash_table_remove_all(di->selection);
}

void di_moveTo(DrawImage *di, gdouble imgXRef, gdouble imgYRef)
{
    DrawImageState *state = getStateForModify(di, SM_IMAGE_SIZE);
    state->imgXRef = imgXRef;
    state->imgYRef = imgYRef;
    g_hash_table_remove_all(di->selection);
}

void di_setSize(DrawImage *di, gint imgWidth, gint imgHeight,
        gdouble translateXfactor, gdouble translateYfactor)
{
    DrawImageState *state = getStateForModify(di, SM_IMAGE_SIZE);
    state->imgXRef += translateXfactor * (imgWidth - state->imgWidth);
    state->imgYRef += translateYfactor * (imgHeight - state->imgHeight);
    state->imgWidth = imgWidth;
    state->imgHeight = imgHeight;
    g_hash_table_remove_all(di->selection);
}

void di_draw(const DrawImage *di, cairo_t *cr, gdouble zoom)
{
    cairo_matrix_t matrix;
    double xBeg, yBeg;
    const DrawImageState *state = di->states + di->stateCur;
    gint baseImgWidth, baseImgHeight;
    cairo_surface_t *baseImage;

    gdk_cairo_set_source_rgba(cr, &state->imgBgColor);
    if( state->baseImage != NULL ) {
        baseImage = di->preview ? di->preview : state->baseImage;
        baseImgWidth = cairo_image_surface_get_width(baseImage);
        baseImgHeight = cairo_image_surface_get_height(baseImage);
        if( zoom != 1.0 ) {
            cairo_save(cr);
            cairo_scale(cr, zoom, zoom);
        }
        if( state->imgXRef > 0 && state->imgYRef + baseImgHeight > 0 ) {
            cairo_rectangle(cr, 0, 0, fmin(state->imgXRef, state->imgWidth),
                    fmin(state->imgYRef + baseImgHeight, state->imgHeight));
            cairo_fill(cr);
        }
        if( state->imgXRef < state->imgWidth && state->imgYRef > 0 ) {
            xBeg = fmax(state->imgXRef, 0);
            cairo_rectangle(cr, xBeg, 0, state->imgWidth - xBeg,
                    fmin(state->imgYRef, state->imgHeight) );
            cairo_fill(cr);
        }
        xBeg = fmax(state->imgXRef + baseImgWidth, 0);
        if( xBeg < state->imgWidth && state->imgYRef < state->imgHeight ) {
            yBeg = fmax(state->imgYRef, 0);
            cairo_rectangle(cr, xBeg, yBeg, state->imgWidth - xBeg,
                    state->imgHeight - yBeg);
            cairo_fill(cr);
        }
        yBeg = fmax(state->imgYRef + baseImgHeight, 0);
        if( state->imgXRef + baseImgWidth > 0 && yBeg < state->imgHeight ) {
            cairo_rectangle(cr, 0, yBeg,
                    fmin(state->imgXRef + baseImgWidth, state->imgWidth),
                    state->imgHeight - yBeg);
            cairo_fill(cr);
        }
        cairo_set_source_surface(cr, baseImage,
            state->imgXRef, state->imgYRef);
        cairo_paint(cr);
        if( zoom != 1.0 )
            cairo_restore(cr);
    }else{
        cairo_paint(cr);
    }
    if( state->imgXRef != 0.0 || state->imgYRef != 0.0 ) {
        cairo_save(cr);
        cairo_translate(cr, zoom * state->imgXRef, zoom * state->imgYRef);
    }
    for(int i = 0; i < state->shapeCount; ++i)
        shape_draw(state->shapes[i], cr, zoom,
                g_hash_table_contains(di->selection, GINT_TO_POINTER(i)),
                i == di->curShapeIdx);
    if( state->imgXRef != 0.0 || state->imgYRef != 0.0 )
        cairo_restore(cr);
}

void di_thresholdPreview(DrawImage *di, gdouble level)
{
    DrawImageState *state = di->states + di->stateCur;
    int i, j, imgWidth, imgHeight, srcStride, destStride;
    const unsigned char *src, *pxsrc;
    unsigned char *dest, *pxdest;

    if( state->baseImage == NULL )
        return;
    imgWidth = cairo_image_surface_get_width(state->baseImage);
    imgHeight = cairo_image_surface_get_height(state->baseImage);
    srcStride = cairo_image_surface_get_stride(state->baseImage);
    cairo_surface_flush(state->baseImage);
    src = cairo_image_surface_get_data(state->baseImage);
    if( di->preview == NULL ) {
        di->preview = cairo_image_surface_create(
                CAIRO_FORMAT_ARGB32, imgWidth, imgHeight);
    }
    cairo_surface_flush(di->preview);
    destStride = cairo_image_surface_get_stride(di->preview);
    dest = cairo_image_surface_get_data(di->preview);
    for(i = 0; i < imgHeight; ++i) {
        pxsrc = src;
        pxdest = dest;
        for(j = 0; j < imgWidth; ++j) {
            if( 3 * pxsrc[3] * (1 - level) >= pxsrc[0] + pxsrc[1] + pxsrc[2] )
                pxdest[0] = pxdest[1] = pxdest[2] = 0;
            else
                pxdest[0] = pxdest[1] = pxdest[2] = pxsrc[3];
            pxdest[3] = pxsrc[3];
            pxsrc += 4;
            pxdest += 4;
        }
        src += srcStride;
        dest += destStride;
    }
    cairo_surface_mark_dirty(di->preview);
}

void di_thresholdFinish(DrawImage *di, gboolean commit)
{
    DrawImageState *state = di->states + di->stateCur;

    if( state->baseImage == NULL || di->preview == NULL )
        return;
    if( commit ) {
        state = getStateForModify(di, SM_IMAGE_THRESHOLD);
        cairo_surface_destroy(state->baseImage);
        state->baseImage = di->preview;
    }else{
        cairo_surface_destroy(di->preview);
    }
    di->preview = NULL;
}

gboolean di_saveWLQ(DrawImage *di, const char *fileName, gchar **errLoc)
{
    const DrawImageState *state = di->states + di->stateCur;
    int baseImgWidth = 0, baseImgHeight = 0, baseImgStride = 0, i;
    const unsigned char *data;
    WlqOutFile *outFile;

    if( (outFile = wlq_openOut(fileName, errLoc)) == NULL )
        return FALSE;
    if( state->baseImage != NULL ) {
        baseImgWidth = cairo_image_surface_get_width(state->baseImage);
        baseImgHeight = cairo_image_surface_get_height(state->baseImage);
        baseImgStride = cairo_image_surface_get_stride(state->baseImage);
    }
    gboolean isOK = wlq_writeU32(outFile, state->imgWidth, errLoc)
            && wlq_writeU32(outFile, state->imgHeight, errLoc)
            && wlq_writeCoordinate(outFile, state->imgXRef, errLoc)
            && wlq_writeCoordinate(outFile, state->imgYRef, errLoc)
            && wlq_writeRGBA(outFile, &state->imgBgColor, errLoc)
            && wlq_writeU32(outFile, baseImgWidth, errLoc)
            && wlq_writeU32(outFile, baseImgHeight, errLoc)
            && wlq_writeU32(outFile, baseImgStride, errLoc);
    if( isOK && state->baseImage != NULL ) {
        cairo_surface_flush(state->baseImage);
        data = cairo_image_surface_get_data(state->baseImage);
        isOK = wlq_write(outFile, data, baseImgHeight * baseImgStride, errLoc);
    }
    if( isOK ) {
        isOK = wlq_writeU32(outFile, state->shapeCount, errLoc);
        for(i = 0; i < state->shapeCount && isOK; ++i)
            isOK = shape_writeToFile(state->shapes[i], outFile, errLoc);
    }
    wlq_closeOut(outFile);
    return isOK;
}

void di_markSaved(DrawImage *di)
{
    di->savedStateId = di->states[di->stateCur].id;
}

gboolean di_isModified(const DrawImage *di)
{
    const DrawImageState *state = di->states + di->stateCur;

    return state->id != di->savedStateId;
}

void di_free(DrawImage *di)
{
    freeStates(di->states, di->stateFirst, di->stateLast);
    g_hash_table_unref(di->selection);
    g_hash_table_unref(di->addedByRectSel);
    if( di->preview ) {
        g_warning("di_free: dangling image preview");
        cairo_surface_destroy(di->preview);
    }
    g_free(di);
}
