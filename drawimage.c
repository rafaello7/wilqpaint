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
    SM_SHAPE_LAYOUT_NEW,    /* add a new shape and begin layout */
    SM_SHAPE_LAYOUT,        /* change layout of current shape */
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
    enum ShapeSide dragShapeSide;
    guint nextStateId;
    guint savedStateId;
    GHashTable *selection;
    GHashTable *addedByRectSel;
    gdouble selXRef, selYRef;
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
    di->curStateModification = SM_NEW;
    di->curShapeIdx = -1;
    di->dragShapeSide = SS_NONE;
    di->nextStateId = 1;
    di->savedStateId = 0;
    di->selection = g_hash_table_new(NULL, NULL);
    di->addedByRectSel = g_hash_table_new(NULL, NULL);
    di->selXRef = 0;
    di->selYRef = 0;
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

static DrawImage *openAsWLQ(const char *fileName, gchar **errLoc,
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
        di = di_new(imgWidth, imgHeight);
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

static gboolean saveAsWLQ(DrawImage *di, const char *fileName, gchar **errLoc)
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

DrawImage *di_open(const char *fileName, gchar **errLoc, gboolean *isNoEntErr)
{
    DrawImage *di = NULL;
    int nameLen = strlen(fileName);
    GError *gerr = NULL;

    if( nameLen >= 4 && !strcasecmp(fileName + nameLen - 4, ".wlq") ) {
        di = openAsWLQ(fileName, errLoc, isNoEntErr);
    }else{
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(fileName, &gerr);
        if( pixbuf != NULL ) {
            di = di_new(gdk_pixbuf_get_width(pixbuf),
                    gdk_pixbuf_get_height(pixbuf));
            DrawImageState *state = di->states;
            state->baseImage = cairo_image_surface_create(
                    CAIRO_FORMAT_ARGB32, state->imgWidth, state->imgHeight);
            cairo_t *cr = cairo_create(state->baseImage);
            gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);
            g_object_unref(G_OBJECT(pixbuf));
        }else{
            *isNoEntErr =
                    g_error_matches(gerr, G_FILE_ERROR, G_FILE_ERROR_NOENT)
                || g_error_matches(gerr, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);
            *errLoc = g_strdup(gerr->message);
            g_error_free(gerr);
        }
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
    DrawImageState *state = getStateForModify(di, SM_IMAGE_BACKGROUND);
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
    DrawImageState *state;

    state = getStateForModify(di, SM_SHAPE_LAYOUT_NEW);
    shape = shape_new(shapeType, xRef - state->imgXRef, yRef - state->imgYRef,
            shapeParams);
    di->curShapeIdx = state->shapeCount;
    state->shapes = g_realloc(state->shapes,
            ++state->shapeCount * sizeof(Shape*));
    state->shapes[di->curShapeIdx] = shape;
    di->selXRef = xRef;
    di->selYRef = yRef;
    g_hash_table_remove_all(di->selection);
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

gboolean di_selectionFromPoint(DrawImage *di, gdouble x, gdouble y,
        gboolean isRect, gboolean extend)
{
    DrawImageState *state = di->states + di->stateCur;
    int shapeIdx;
    gpointer idxAsPtr;
    enum ShapeSide side;

    g_hash_table_remove_all(di->addedByRectSel);
    if( ! extend )
        g_hash_table_remove_all(di->selection);
    di->curShapeIdx = -1;
    di->curStateModification = SM_SELECTION_MARK;
    di->selXRef = x;
    di->selYRef = y;
    if( isRect ) {
        for( shapeIdx = state->shapeCount - 1; shapeIdx >= 0; --shapeIdx ) {
            idxAsPtr = GINT_TO_POINTER(shapeIdx);
            if( ! g_hash_table_contains(di->selection, idxAsPtr)
                && (side = shape_hitTest(state->shapes[shapeIdx],
                    x - state->imgXRef, y - state->imgYRef,
                    x - state->imgXRef, y - state->imgYRef)) != SS_NONE)
            {
                g_hash_table_add(di->addedByRectSel, idxAsPtr);
                g_hash_table_add(di->selection, idxAsPtr);
            }
        }
    }else{
        shapeIdx = state->shapeCount - 1;
        while( shapeIdx >= 0 && di->curShapeIdx == -1 ) {
            if( (side = shape_hitTest(state->shapes[shapeIdx],
                    x - state->imgXRef, y - state->imgYRef,
                    x - state->imgXRef, y - state->imgYRef)) != SS_NONE )
            {
                di->curShapeIdx = shapeIdx;
                idxAsPtr = GINT_TO_POINTER(shapeIdx);
                if( ! g_hash_table_contains(di->selection, idxAsPtr) )
                    g_hash_table_add(di->selection, idxAsPtr);
                if( ! extend && side != SS_MID ) {
                    di->curStateModification = SM_SHAPESIDE_MARK;
                    di->dragShapeSide = side;
                }
            }
            --shapeIdx;
        }
    }
    return di->curShapeIdx != -1;
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

void di_selectionDragTo(DrawImage *di, gdouble x, gdouble y)
{
    GHashTableIter iter;
    gpointer key;
    DrawImageState *state, *stPrev;

    switch( di->curStateModification ) {
    case SM_SHAPE_LAYOUT_NEW:
        state = getStateForModify(di, SM_SHAPE_LAYOUT_NEW);
        shape_layoutNew(state->shapes[di->curShapeIdx],
                x - state->imgXRef, y - state->imgYRef);
        break;
    case SM_SHAPESIDE_MARK:
    case SM_SHAPE_LAYOUT:
        state = getStateForModify(di, SM_SHAPE_LAYOUT);
        stPrev = di->states + (di->stateCur == 0 ? UNDO_MAX : di->stateCur) - 1;
        shape_layout(state->shapes[di->curShapeIdx],
                stPrev->shapes[di->curShapeIdx],
                x - di->selXRef, y - di->selYRef, di->dragShapeSide);
        break;
    case SM_SELECTION_MARK:
    case SM_SEL_DRAG:
        if( g_hash_table_size(di->selection) > 0 ) {
            state = getStateForModify(di, SM_SEL_DRAG);
            stPrev = di->states
                    + (di->stateCur == 0 ? UNDO_MAX : di->stateCur) - 1;
            g_hash_table_iter_init(&iter, di->selection);
            while( g_hash_table_iter_next(&iter, &key, NULL) ) {
                gint shapeIdx = GPOINTER_TO_INT(key);
                shape_layout(state->shapes[shapeIdx],
                        stPrev->shapes[shapeIdx],
                        x - di->selXRef, y - di->selYRef, SS_MID);
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

void di_draw(const DrawImage *di, cairo_t *cr)
{
    cairo_matrix_t matrix;
    double xBeg, yBeg;
    const DrawImageState *state = di->states + di->stateCur;
    gint baseImgWidth, baseImgHeight;

    gdk_cairo_set_source_rgba(cr, &state->imgBgColor);
    if( state->baseImage != NULL ) {
        baseImgWidth = cairo_image_surface_get_width(state->baseImage);
        baseImgHeight = cairo_image_surface_get_height(state->baseImage);
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
        cairo_set_source_surface(cr, state->baseImage,
            state->imgXRef, state->imgYRef);
        cairo_paint(cr);
    }else{
        cairo_paint(cr);
    }
    if( state->imgXRef != 0.0 || state->imgYRef != 0.0 ) {
        cairo_save(cr);
        cairo_translate(cr, state->imgXRef, state->imgYRef);
    }
    for(int i = 0; i < state->shapeCount; ++i)
        shape_draw(state->shapes[i], cr, i == di->curShapeIdx,
                g_hash_table_contains(di->selection, GINT_TO_POINTER(i)));
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

gboolean di_save(DrawImage *di, const char *fileName, gchar **errLoc)
{
    const DrawImageState *state = di->states + di->stateCur;
    int nameLen = strlen(fileName);
    GError *gerr = NULL;
    gboolean isOK;

    g_hash_table_remove_all(di->selection);
    di->curShapeIdx = -1;
    if( nameLen >= 4 && !strcasecmp(fileName + nameLen - 4, ".wlq") ) {
        isOK = saveAsWLQ(di, fileName, errLoc);
    }else{
        cairo_surface_t *paintImage = cairo_image_surface_create(
                CAIRO_FORMAT_ARGB32, state->imgWidth, state->imgHeight);
        cairo_t *cr = cairo_create(paintImage);
        di_draw(di, cr);
        cairo_destroy(cr);
        GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(paintImage,
                0, 0, state->imgWidth, state->imgHeight);
        cairo_surface_destroy(paintImage);
        isOK = gdk_pixbuf_save(pixbuf, fileName,
                getFileFormatByExt(fileName), &gerr, NULL);
        g_object_unref(G_OBJECT(pixbuf));
        if( ! isOK ) {
            *errLoc = g_strdup(gerr->message);
            g_error_free(gerr);
        }
    }
    if( isOK )
        di->savedStateId = state->id;
    return isOK;
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
    g_free(di);
}

