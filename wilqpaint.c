#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "sizedialog.h"
#include "griddialog.h"
#include "quitdialog.h"
#include "aboutdialog.h"
#include "drawimage.h"
#include "colorchooser.h"


typedef enum {
    SA_LAYOUT,
    SA_SELECTAREA,
    SA_MOVEIMAGE
} ShapeAction;

static char *curFileName = NULL;
static DrawImage *drawImage;
static ShapeAction curAction;
static gdouble moveXref, moveYref;
static gdouble selXref, selYref, selWidth, selHeight;
static GtkBuilder *builder;
static gdouble curZoom = 1.0;
static gint shapeControlsSetInProgress = 0;


static gboolean isToggleButtonActive(const char *toggleButtonName)
{
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
            gtk_builder_get_object(builder, toggleButtonName)));
}

static gdouble getSpinButtonValue(const char *spinButtonName)
{
    return gtk_spin_button_get_value(GTK_SPIN_BUTTON(
            gtk_builder_get_object(builder, spinButtonName)));
}

static void setSpinButtonValue(const char *spinButtonName, gdouble value)
{
    ++shapeControlsSetInProgress;
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(
            gtk_builder_get_object(builder, spinButtonName)), value);
    --shapeControlsSetInProgress;
}

static void setSpinButtonRange(const char *spinButtonName, gdouble min,
        gdouble max)
{
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(
            gtk_builder_get_object(builder, spinButtonName)), min, max);
}

static void setCurFileName(const char *fname)
{
    GtkWindow *window;
    char title[128];
    const char *baseName;
    int len;

    g_free(curFileName);
    curFileName = fname == NULL ? NULL : g_strdup(fname);
    window = GTK_WINDOW(gtk_builder_get_object(builder, "mainWindow"));
    if( fname != NULL ) {
        if( (baseName = strrchr(fname, '/')) == NULL )
            baseName = fname;
        else
            ++baseName;
    }else
        baseName = "Unnamed";
    title[0] = '\0';
    if( (len = strlen(baseName)) > 100 ) {
        baseName += len - 100;
        strcpy(title, "...");
    }
    sprintf(title + strlen(title), "%s - wilqpaint", baseName);
    gtk_window_set_title(window, title);
}

static void redrawDrawing(void)
{
    GtkWidget *drawing = GTK_WIDGET(gtk_builder_get_object(builder,
                "drawing"));
    GdkRectangle update_rect;
    update_rect.x = 0;
    update_rect.y = 0;
    update_rect.width = gtk_widget_get_allocated_width (drawing);
    update_rect.height = gtk_widget_get_allocated_height (drawing);
    GdkWindow *gdkWin = gtk_widget_get_window (drawing);
    gdk_window_invalidate_rect (gtk_widget_get_window(drawing),
            &update_rect, FALSE);
}

void adjustDrawingSize(gboolean adjustImageSizeSpins)
{
    GtkWidget *drawing;
    gint imgWidth, imgHeight;

    selWidth = selHeight = 0;
    drawing = GTK_WIDGET(gtk_builder_get_object(builder, "drawing"));
    imgWidth = di_getWidth(drawImage);
    imgHeight = di_getHeight(drawImage);
    gtk_widget_set_size_request(drawing, imgWidth * curZoom,
            imgHeight * curZoom);
    if( adjustImageSizeSpins ) {
        setSpinButtonValue("spinImageWidth", imgWidth);
        setSpinButtonValue("spinImageHeight", imgHeight);
    }
}

static void redrawShapePreview(void)
{
    GtkWidget *drawing = GTK_WIDGET(gtk_builder_get_object(builder,
                "shapePreview"));
    GdkRectangle update_rect;
    update_rect.x = 0;
    update_rect.y = 0;
    update_rect.width = gtk_widget_get_allocated_width (drawing);
    update_rect.height = gtk_widget_get_allocated_height (drawing);
    GdkWindow *gdkWin = gtk_widget_get_window (drawing);
    gdk_window_invalidate_rect (gtk_widget_get_window(drawing),
            &update_rect, FALSE);
}

static void adjustBackgroundColorControl(void)
{
    GtkColorChooser *bgColorButton;
    GdkRGBA color;

    bgColorButton = GTK_COLOR_CHOOSER(gtk_builder_get_object(builder,
                "backgroundColor"));
    di_getBackgroundColor(drawImage, &color);
    gtk_color_chooser_set_rgba(bgColorButton, &color);
}

static void setControlsFromShapeParams(const ShapeParams *shapeParams)
{
    GtkColorChooser *colorBtn;
    GtkTextView *textView;
    GtkTextBuffer *textBuffer;
    GtkFontButton *fontButton;
   
    ++shapeControlsSetInProgress;
    colorBtn = GTK_COLOR_CHOOSER(gtk_builder_get_object(builder,
                "strokeColor"));
    gtk_color_chooser_set_rgba(colorBtn, &shapeParams->strokeColor);
    colorBtn = GTK_COLOR_CHOOSER(gtk_builder_get_object(builder,
                "fillColor"));
    gtk_color_chooser_set_rgba(colorBtn, &shapeParams->fillColor);
    colorBtn = GTK_COLOR_CHOOSER(gtk_builder_get_object(builder,
                "textColor"));
    gtk_color_chooser_set_rgba(colorBtn, &shapeParams->textColor);
    setSpinButtonValue("thickness", shapeParams->thickness);
    setSpinButtonValue("angle", shapeParams->angle);
    setSpinButtonValue("round", shapeParams->round);
    textView = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "text"));
    textBuffer = gtk_text_view_get_buffer(textView);
    if( shapeParams->text ) {
        gtk_text_buffer_set_text(textBuffer, shapeParams->text, -1);
        fontButton = GTK_FONT_BUTTON(
                gtk_builder_get_object(builder, "font"));
        gtk_font_button_set_font_name(fontButton, shapeParams->fontName);
    }else{
        gtk_text_buffer_set_text(textBuffer, "", 0);
    }
    --shapeControlsSetInProgress;
}

static void getShapeParamsFromControls(ShapeParams *shapeParams)
{
    GtkColorChooser *colorBtn;
    GtkTextView *textView;
    GtkTextBuffer *textBuffer;
    GtkTextIter start, end;
    GtkFontButton *fontButton;
   
    colorBtn = GTK_COLOR_CHOOSER(gtk_builder_get_object(builder,
                "strokeColor"));
    gtk_color_chooser_get_rgba(colorBtn, &shapeParams->strokeColor);
    colorBtn = GTK_COLOR_CHOOSER(gtk_builder_get_object(builder,
                "fillColor"));
    gtk_color_chooser_get_rgba(colorBtn, &shapeParams->fillColor);
    colorBtn = GTK_COLOR_CHOOSER(gtk_builder_get_object(builder,
                "textColor"));
    gtk_color_chooser_get_rgba(colorBtn, &shapeParams->textColor);
    shapeParams->thickness = getSpinButtonValue("thickness");
    shapeParams->angle = getSpinButtonValue("angle");
    shapeParams->round = getSpinButtonValue("round");
    textView = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "text"));
    textBuffer = gtk_text_view_get_buffer(textView);
    gtk_text_buffer_get_bounds (textBuffer, &start, &end);
    shapeParams->text =
        gtk_text_buffer_get_text(textBuffer, &start, &end, FALSE);
    fontButton = GTK_FONT_BUTTON(gtk_builder_get_object(builder, "font"));
    shapeParams->fontName = gtk_font_button_get_font_name(fontButton);
}

void on_shapePreview_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    ShapeParams shapeParams;
    Shape *shape = NULL;
    int width, height, i;
    enum ShapeSide side = SS_RIGHT | SS_BOTTOM | SS_CREATE;
    gdouble thickness, round;

    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);
    if( width <= 20 || height <= 20 )
        return;
    getShapeParamsFromControls(&shapeParams);
    g_free((void*)shapeParams.text);
    shapeParams.text = NULL;
    thickness = shapeParams.thickness;
    round = shapeParams.round;
    if( isToggleButtonActive("shapeFreeForm") ) {
        shape = shape_new(ST_FREEFORM, 0, height / 6, &shapeParams);
        for(i = 0; i < width; i += 4) {
            shape_moveTo(shape, i, 0.20 * height
                    * (3.0 - cos(53.0 * G_PI / width)
                        - 4.0 * fabs(i - 0.5 * width) / width
                        - cos(5.3 * (i - 10) / width * G_PI)), side);
        }
    }else if( isToggleButtonActive("shapeLine") ) {
        shape = shape_new(ST_LINE, 2, height/2, &shapeParams);
        shape_moveTo(shape, width - 4, 0, side);
    }else if( isToggleButtonActive("shapeTriangle") ) {
        gdouble h, wtoh = 2.0 * tan(shapeParams.angle * G_PI / 360);
        h = (height - thickness - 8) * wtoh > width - thickness - 4 ?
            (width - thickness - 8) / wtoh : height - thickness - 4;
        shape = shape_new(ST_TRIANGLE, width/2,
                0.5 * (height - h), &shapeParams);
        shape_moveTo(shape, 0, h, side);
    }else if( isToggleButtonActive("shapeRect") ) {
        shape = shape_new(ST_RECT,
                2 + 0.5 * thickness + 0.5 * round * (2 - G_SQRT2),
                2 + 0.5 * thickness + 0.5 * round * (2 - G_SQRT2),
                &shapeParams);
        shape_moveTo(shape,
                width - 4 - thickness - round * (2 - G_SQRT2),
                height - 4 - thickness - round * (2 - G_SQRT2), side);
    }else if( isToggleButtonActive("shapeOval") ) {
        shape = shape_new(ST_OVAL,
                0.25 * (width - thickness - 4) * (2 - G_SQRT2)
                + 0.5 * thickness + 2,
                0.25 * (height - thickness - 4) * (2 - G_SQRT2)
                + 0.5 * thickness + 2,
                &shapeParams);
        shape_moveTo(shape,
                0.5 * G_SQRT2 * (width - thickness - 4),
                0.5 * G_SQRT2 * (height - thickness - 4), side);
    }else if( isToggleButtonActive("shapeText") ) {
        shapeParams.text = "Ww";
        shape = shape_new(ST_TEXT, 10, 10, &shapeParams);
        shape_moveTo(shape, width/2 - 10, height/2 - 10, side);
    }else{
        shape = shape_new(ST_ARROW, 2, height/2, &shapeParams);
        shape_moveTo(shape, width - 4, 0, side);
    }
    if( shape != NULL ) {
        shape_draw(shape, cr, FALSE, FALSE);
        shape_unref(shape);
    }
}

void on_thickness_value_changed(GtkSpinButton *spin, gpointer user_data)
{
    ShapeParams shapeParams;

    if( shapeControlsSetInProgress == 0 ) {
        shapeParams.thickness = gtk_spin_button_get_value(spin);
        di_setSelectionParam(drawImage, SP_THICKNESS, &shapeParams);
        redrawDrawing();
        redrawShapePreview();
    }
}

void on_round_value_changed(GtkSpinButton *spin, gpointer user_data)
{
    ShapeParams shapeParams;

    if( shapeControlsSetInProgress == 0 ) {
        shapeParams.round = gtk_spin_button_get_value(spin);
        di_setSelectionParam(drawImage, SP_ROUND, &shapeParams);
        redrawDrawing();
        redrawShapePreview();
    }
}

void on_angle_value_changed(GtkSpinButton *spin, gpointer user_data)
{
    ShapeParams shapeParams;

    if( shapeControlsSetInProgress == 0 ) {
        shapeParams.angle = gtk_spin_button_get_value(spin);
        di_setSelectionParam(drawImage, SP_ANGLE, &shapeParams);
        redrawDrawing();
        redrawShapePreview();
    }
}

void on_shapeTextBuffer_changed(GtkTextBuffer *textBuffer, gpointer user_data)
{
    ShapeParams shapeParams;
    GtkTextIter start, end;
    GtkFontButton *fontButton;

    if( shapeControlsSetInProgress == 0 ) {
        gtk_text_buffer_get_bounds (textBuffer, &start, &end);
        shapeParams.text =
            gtk_text_buffer_get_text(textBuffer, &start, &end, FALSE);
        fontButton = GTK_FONT_BUTTON(gtk_builder_get_object(builder, "font"));
        shapeParams.fontName = gtk_font_button_get_font_name(fontButton);
        di_setSelectionParam(drawImage, SP_TEXT, &shapeParams);
        g_free((char*)shapeParams.text);
        redrawDrawing();
    }
}

void on_font_font_set(GtkFontButton *fontButton, gpointer user_data)
{
    ShapeParams shapeParams;

    if( shapeControlsSetInProgress == 0 ) {
        shapeParams.fontName = gtk_font_button_get_font_name(fontButton);
        di_setSelectionParam(drawImage, SP_FONTNAME, &shapeParams);
        redrawDrawing();
    }
}

void on_textColor_color_set(GtkColorButton *colorButton, gpointer user_data)
{
    ShapeParams shapeParams;

    if( shapeControlsSetInProgress == 0 ) {
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(colorButton),
                &shapeParams.textColor);
        di_setSelectionParam(drawImage, SP_TEXTCOLOR, &shapeParams);
        redrawDrawing();
    }
}

void on_strokeColor_color_set(GtkColorButton *colorButton, gpointer user_data)
{
    ShapeParams shapeParams;

    if( shapeControlsSetInProgress == 0 ) {
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(colorButton),
                &shapeParams.textColor);
        di_setSelectionParam(drawImage, SP_STROKECOLOR, &shapeParams);
        redrawDrawing();
        redrawShapePreview();
    }
}

void on_fillColor_color_set(GtkColorButton *colorButton, gpointer user_data)
{
    ShapeParams shapeParams;

    if( shapeControlsSetInProgress == 0 ) {
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(colorButton),
                &shapeParams.textColor);
        di_setSelectionParam(drawImage, SP_FILLCOLOR, &shapeParams);
        redrawDrawing();
        redrawShapePreview();
    }
}

void onShapeColorChosen(enum ChosenColor cc)
{
    ShapeParams shapeParams;
    GtkColorChooser *colorBtn;
    GdkRGBA color;

    colorBtn = GTK_COLOR_CHOOSER(gtk_builder_get_object(builder,
                cc == CC_STROKE ? "strokeColor" : "fillColor"));
    gtk_color_chooser_get_rgba(colorBtn, &color);
    if( cc == CC_STROKE ) {
        shapeParams.strokeColor = color;
        di_setSelectionParam(drawImage, SP_STROKECOLOR, &shapeParams);
    }else{
        shapeParams.fillColor = color;
        di_setSelectionParam(drawImage, SP_FILLCOLOR, &shapeParams);
    }
    redrawDrawing();
    redrawShapePreview();
}

static void setZoom1x(void)
{
    GActionMap *mainWin;
    GAction *zoomAction;
    GVariant *state;

    mainWin = G_ACTION_MAP(gtk_builder_get_object(builder, "mainWindow"));
    zoomAction = g_action_map_lookup_action(mainWin, "image-zoom");
    state = g_variant_new_string("1");
    g_action_change_state(zoomAction, state);
    curZoom = 1.0;
}

static gdouble snapXValue(gdouble val)
{
    return grid_isSnapTo() ? grid_getSnapXValue(val) : val;
}

static gdouble snapYValue(gdouble val)
{
    return grid_isSnapTo() ? grid_getSnapYValue(val) : val;
}

gboolean on_drawing_button_press(GtkWidget *widget, GdkEventButton *event,
               gpointer user_data)
{
    gdouble evX = snapXValue(event->x / curZoom);
    gdouble evY = snapYValue(event->y / curZoom);
    ShapeType shapeType;
    ShapeParams shapeParams;

    selWidth = selHeight = 0;
    if( isToggleButtonActive("shapeSelect") ) {
        if( event->state & GDK_SHIFT_MASK ) {
            curAction = SA_SELECTAREA;
            selXref = evX;
            selYref = evY;
        }else
            curAction = SA_LAYOUT;
        if( di_selectionFromPoint(drawImage, evX, evY,
                    curAction == SA_SELECTAREA,
                    event->state & GDK_CONTROL_MASK) )
        {
            di_getCurShapeParams(drawImage, &shapeParams);
            setControlsFromShapeParams(&shapeParams);
        }
    }else if( isToggleButtonActive("shapeImageSize") ) {
        moveXref = evX - di_getXRef(drawImage);
        moveYref = evY - di_getYRef(drawImage);
        curAction = SA_MOVEIMAGE;
    }else{
        if( isToggleButtonActive("shapeFreeForm") )
            shapeType = ST_FREEFORM;
        else if( isToggleButtonActive("shapeLine") )
            shapeType = ST_LINE;
        else if( isToggleButtonActive("shapeTriangle") )
            shapeType = ST_TRIANGLE;
        else if( isToggleButtonActive("shapeRect") )
            shapeType = ST_RECT;
        else if( isToggleButtonActive("shapeOval") )
            shapeType = ST_OVAL;
        else if( isToggleButtonActive("shapeText") )
            shapeType = ST_TEXT;
        else
            shapeType = ST_ARROW;
        getShapeParamsFromControls(&shapeParams);
        di_addShape(drawImage, shapeType, evX, evY, &shapeParams);
        g_free((void*)shapeParams.text);
        curAction = SA_LAYOUT;
    }
    redrawDrawing();
    gtk_widget_grab_focus(widget);
    return TRUE;
}

gboolean on_drawing_motion (GtkWidget *widget, GdkEventMotion *event,
               gpointer user_data)
{
    gdouble evX = snapXValue(event->x / curZoom);
    gdouble evY = snapYValue(event->y / curZoom);

    if( event->state & GDK_BUTTON1_MASK ) {
        switch( curAction ) {
        case SA_LAYOUT:
            di_selectionMoveTo(drawImage, evX, evY);
            break;
        case SA_MOVEIMAGE:
            di_moveTo(drawImage, evX - moveXref, evY - moveYref);
            break;
        case SA_SELECTAREA:
            selWidth = evX - selXref;
            selHeight = evY - selYref;
            di_selectionFromRect(drawImage, evX, evY);
            break;
        }
        redrawDrawing();
    }
    return TRUE;
}

gboolean on_drawing_keypress(GtkWidget *widget, GdkEventKey *event,
        gpointer user_data)
{
    switch( event->keyval ) {
    case GDK_KEY_Delete:
        if( di_selectionDelete(drawImage) )
            redrawDrawing();
        break;
    default:
        break;
    }
    return FALSE;
}

gboolean on_drawing_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    gdouble scale, gridXOffset, gridYOffset, dashes[2];
    gint i, imgWidth, imgHeight;

    cairo_save(cr);
    cairo_scale(cr, curZoom, curZoom);
    di_draw(drawImage, cr);
    cairo_restore(cr);
    scale = grid_getScale() * curZoom;
    if( grid_isShow() && scale > 2 ) {
        gridXOffset = grid_getXOffset() * curZoom;
        gridYOffset = grid_getYOffset() * curZoom;
        if( scale < 32 ) {
            cairo_set_line_width(cr, 1);
            dashes[0] = 1;
            dashes[1] = scale - 1;
            cairo_set_dash(cr, dashes, 2, scale - gridYOffset + 0.5);
        }else{
            cairo_set_line_width(cr, 2);
            dashes[0] = 2;
            dashes[1] = scale - 2;
            cairo_set_dash(cr, dashes, 2, scale - gridYOffset + 1);
        }
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        imgWidth = di_getWidth(drawImage);
        imgHeight = di_getHeight(drawImage);
        for(i = gridXOffset; i <= imgWidth * curZoom; i += scale) {
            cairo_move_to(cr, i, 0);
            cairo_line_to(cr, i, imgHeight * curZoom);
            cairo_stroke(cr);
        }
    }
    if( selWidth != 0 || selHeight != 0 ) {
        cairo_rectangle(cr, selXref * curZoom, selYref * curZoom,
                selWidth * curZoom, selHeight * curZoom);
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
    return FALSE;
}

void on_spinImageSize_value_changed(GtkSpinButton *spin, gpointer user_data)
{
    gdouble translateXfactor = 0.5, translateYfactor = 0.5;

    if( shapeControlsSetInProgress == 0 ) {
        if( isToggleButtonActive("marginLeft") )
            translateXfactor = 1.0;
        else if( isToggleButtonActive("marginRight") )
            translateXfactor = 0.0;
        else if( isToggleButtonActive("marginTop") )
            translateYfactor = 1.0;
        else if( isToggleButtonActive("marginBottom") )
            translateYfactor = 0.0;
        di_setSize(drawImage, getSpinButtonValue("spinImageWidth"),
                getSpinButtonValue("spinImageHeight"),
                translateXfactor, translateYfactor);
        adjustDrawingSize(FALSE);
    }
}

typedef enum {
    STP_SHAPE,
    STP_IMAGESIZE
} ShapeToolsPage;

static void setShapeToolsActivePage(ShapeToolsPage stp)
{
    GtkStack *shapeTools;
    const char *pageName;

    shapeTools = GTK_STACK(gtk_builder_get_object(builder, "shapeTools"));
    switch( stp ) {
    case STP_IMAGESIZE:
        pageName = "toolPageImageSize";
        break;
    default:
        pageName = "toolPageShape";
        break;
    }
    gtk_stack_set_visible_child_name(shapeTools, pageName);
    if( di_selectionSetEmpty(drawImage) || selWidth != 0 || selHeight != 0 ) {
        selWidth = selHeight = 0;
        redrawDrawing();
    }
    redrawShapePreview();
}

void on_shapeSelect_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(STP_SHAPE);
}

void on_shapeImageSize_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(STP_IMAGESIZE);
}

void on_shapeFreeForm_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(STP_SHAPE);
}

void on_shapeLine_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(STP_SHAPE);
}

void on_shapeArrow_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(STP_SHAPE);
}

void on_shapeText_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(STP_SHAPE);
}

void on_shapeTriangle_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(STP_SHAPE);
}

void on_shapeRect_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(STP_SHAPE);
}

void on_shapeOval_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(STP_SHAPE);
}

/* Saves the file modifications.
 * If onQuit flag is set to TRUE, the file is saved only when modified.
 * User is also asked for save changes in this case.
 *
 * If forceChooseFileName is TRUE, the user is always asked for file name.
 * If forceChooseFileName is FALSE, the file is saved under current name.
 * User may still be asked for file name if the name is unnamed so far.
 *
 * Returns:
 *      TRUE  - quit operation may be continued
 *      FALSE - quit operation should be canceled
 */
static gboolean saveChanges(gboolean onQuit, gboolean forceChooseFileName)
{
    GtkFileChooser *fileChooser;
    GtkWindow *mainWindow;
    GtkFileFilter *filt;
    const char *fname;
    gboolean doSave = TRUE;

    mainWindow = GTK_WINDOW(gtk_builder_get_object(builder, "mainWindow"));
    if( onQuit ) {
        if( drawImage == NULL || ! di_isModified(drawImage) )
            return TRUE;
        switch( showQuitDialog(mainWindow) ) {
        case GTK_RESPONSE_YES:
            break;
        case GTK_RESPONSE_CANCEL:
            return FALSE;
        default:
            return TRUE;
        }
    }
    if( curFileName == NULL || forceChooseFileName ) {
        fileChooser = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new (
                    "save file - wilqpaint", mainWindow,
                    GTK_FILE_CHOOSER_ACTION_SAVE,
                    "_Cancel", GTK_RESPONSE_CANCEL,
                    "_Save", GTK_RESPONSE_ACCEPT, NULL));
        gtk_file_chooser_set_do_overwrite_confirmation(fileChooser, TRUE);
        if( curFileName != NULL ) {
            const char *slash = strrchr(curFileName, '/');
            if( slash != NULL ) {
                char *dirname = g_strdup(curFileName);
                dirname[slash - curFileName] = '\0';
                gtk_file_chooser_set_current_folder(fileChooser, dirname);
                g_free(dirname);
            }
        }
        filt = gtk_file_filter_new();
        gtk_file_filter_add_mime_type(filt, "image/*");
        gtk_file_chooser_set_filter(fileChooser, filt);
        if( gtk_dialog_run(GTK_DIALOG(fileChooser)) == GTK_RESPONSE_ACCEPT ) {
            fname = gtk_file_chooser_get_filename(fileChooser);
            setCurFileName(fname);
        }else
            doSave = FALSE;
        gtk_widget_destroy(GTK_WIDGET(fileChooser));
    }
    if( doSave )
        di_save(drawImage, curFileName);
    return doSave;
}

/* If fname is NULL, creates a new image with specified size. If the size
 * is 0x0, user is asked for image size.
 *
 * If fname is not null, the file is open. The drawing size is inherited
 * from image.
 *
 * If some image is currently open, user is possibly asked for save changes.
 * Depend on user choice, the file opening may be canceled or changes on
 * current image might be discarded.
 */
void openFile(const char *fname, gdouble imgWidth, gdouble imgHeight)
{
    DrawImage *newDrawImg = NULL;
    GtkWindow *mainWindow;

    if( fname == NULL ) {
        if( imgWidth < 1.0 || imgHeight < 1.0 ) {
            mainWindow = GTK_WINDOW(gtk_builder_get_object(builder,
                        "mainWindow"));
            imgWidth = di_getWidth(drawImage);
            imgHeight = di_getHeight(drawImage);
            if( ! showSizeDialog(mainWindow, "New Image - wilqpaint",
                        &imgWidth, &imgHeight, FALSE) )
                return;
        }
        newDrawImg = di_new(imgWidth, imgHeight);
    }else
        newDrawImg = di_open(fname);
    if( newDrawImg != NULL ) {
        if( drawImage != NULL )
            di_free(drawImage);
        drawImage = newDrawImg;
        setCurFileName(fname);
        setZoom1x();
        adjustDrawingSize(TRUE);
        adjustBackgroundColorControl();
    }
}

gboolean on_mainWindow_delete_event(GtkWidget *widget, GdkEvent *event,
        gpointer user_data)
{
    return ! saveChanges(TRUE, FALSE);
}

static void on_menu_new(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    if( ! saveChanges(TRUE, FALSE) )
        return;
    openFile(NULL, 0, 0);
}

static void on_menu_open(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    GtkWidget *fileChooser;
    GtkWindow *mainWindow;
    GtkFileFilter *filt;
    gint res;
    const char *fname;

    if( ! saveChanges(TRUE, FALSE) )
        return;
    mainWindow = GTK_WINDOW(gtk_builder_get_object(builder, "mainWindow"));
    fileChooser = gtk_file_chooser_dialog_new ("open file - wilqpaint",
                             mainWindow, GTK_FILE_CHOOSER_ACTION_OPEN,
                             "_Cancel", GTK_RESPONSE_CANCEL,
                             "_Open", GTK_RESPONSE_ACCEPT, NULL);
    if( curFileName != NULL ) {
        const char *slash = strrchr(curFileName, '/');
        if( slash != NULL ) {
            char *dirname = g_strdup(curFileName);
            dirname[slash - curFileName] = '\0';
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fileChooser),
                dirname);
            g_free(dirname);
        }
    }
    filt = gtk_file_filter_new();
    gtk_file_filter_add_mime_type(filt, "image/*");
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(fileChooser), filt);
    res = gtk_dialog_run(GTK_DIALOG(fileChooser));
    if( res == GTK_RESPONSE_ACCEPT ) {
        fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fileChooser));
        openFile(fname, 0, 0);
    }
    gtk_widget_destroy(fileChooser);
}

static void on_menu_saveas(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    saveChanges(FALSE, TRUE);
}

static void on_menu_save(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    saveChanges(FALSE, FALSE);
}

static void on_menu_quit(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    gtk_window_close(GTK_WINDOW(window));
}

static void on_menu_edit_undo(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    gint imgWidth = di_getWidth(drawImage);
    gint imgHeight = di_getHeight(drawImage);
    di_undo(drawImage);
    if( di_getWidth(drawImage) != imgWidth
            || di_getHeight(drawImage) != imgHeight )
        adjustDrawingSize(TRUE);
    else
        redrawDrawing();
    adjustBackgroundColorControl();
}

static void on_menu_edit_redo(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    gint imgWidth = di_getWidth(drawImage);
    gint imgHeight = di_getHeight(drawImage);
    di_redo(drawImage);
    if( di_getWidth(drawImage) != imgWidth
            || di_getHeight(drawImage) != imgHeight )
        adjustDrawingSize(TRUE);
    else
        redrawDrawing();
    adjustBackgroundColorControl();
}

static void on_menu_image_scale(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    GtkWindow *mainWindow;
    gdouble imgWidth, imgHeight;

    imgWidth = di_getWidth(drawImage);
    imgHeight = di_getHeight(drawImage);
    mainWindow = GTK_WINDOW(gtk_builder_get_object(builder, "mainWindow"));
    if( showSizeDialog(mainWindow, "Scale image - wilqpaint",
                &imgWidth, &imgHeight, TRUE) )
    {
        di_scale(drawImage, imgWidth / di_getWidth(drawImage));
        adjustDrawingSize(TRUE);
    }
}

void on_backgroundColor_color_set(GtkColorButton *button, gpointer user_data)
{
    GtkWindow *mainWindow;
    GdkRGBA color;

    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &color);
    di_setBackgroundColor(drawImage, &color);
    redrawDrawing();
}

static void menu_image_set_zoom(GSimpleAction *action, GVariant *state,
                    gpointer user_data)
{
    curZoom = strtod(g_variant_get_string(state, NULL), NULL);
    g_simple_action_set_state(action, state);
    adjustDrawingSize(TRUE);
}

static void onGridDialogChange(void)
{
    GActionMap *actionMap;
    GAction *action;
    GVariant *state;
   
    actionMap = G_ACTION_MAP(gtk_builder_get_object(builder,
                "mainWindow"));
    action = g_action_map_lookup_action(actionMap, "grid-show");
    g_action_change_state(action, g_variant_new_boolean(grid_isShow()));
    action = g_action_map_lookup_action(actionMap, "grid-snap");
    g_action_change_state(action, g_variant_new_boolean(grid_isSnapTo()));
    redrawDrawing();
}

static void on_menu_grid_options(GSimpleAction *action, GVariant *parameter,
        gpointer user_data)
{
    GtkWindow *mainWindow;

    mainWindow = GTK_WINDOW(gtk_builder_get_object(builder, "mainWindow"));
    grid_showDialog(mainWindow, onGridDialogChange);
}

static void menu_grid_show (GSimpleAction *action, GVariant *state,
        gpointer user_data)
{
    grid_setIsShow(g_variant_get_boolean(state));
    g_simple_action_set_state (action, state);
    redrawDrawing();
}

static void menu_grid_snap (GSimpleAction *action, GVariant *state,
        gpointer user_data)
{
    grid_setIsSnapTo(g_variant_get_boolean (state));
    g_simple_action_set_state(action, state);
}

static void on_menu_about(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    GtkWindow *mainWindow;

    mainWindow = GTK_WINDOW(gtk_builder_get_object(builder, "mainWindow"));
    showAboutDialog(mainWindow);
}

static void initializeApp(GtkApplication* app, const char *fname)
{
    static GActionEntry win_entries[] =
    {
        /* File */
        { "new",  on_menu_new,  NULL, NULL, NULL },
        { "open", on_menu_open, NULL, NULL, NULL },
        { "save", on_menu_save, NULL, NULL, NULL },
        { "saveas", on_menu_saveas, NULL, NULL, NULL },
        { "quit", on_menu_quit, NULL, NULL, NULL },
        /* Edit */
        { "edit-undo", on_menu_edit_undo, NULL, NULL, NULL },
        { "edit-redo", on_menu_edit_redo, NULL, NULL, NULL },
        { "image-scale", on_menu_image_scale, NULL, NULL, NULL },
        /* View */
        { "image-zoom", NULL, "s", "'1'", menu_image_set_zoom },
        { "grid-options", on_menu_grid_options, NULL, NULL, NULL },
        { "grid-show", NULL, NULL, "false", menu_grid_show },
        { "grid-snap", NULL, NULL, "false", menu_grid_snap },
        /* Help */
        { "help-about",  on_menu_about,  NULL, NULL, NULL }
    };
    GtkWidget *window, *drawing;
    GtkBuilder *menuBld;
    GMenuModel *menuBar;
    gint imgWidth, imgHeight;

    if( builder != NULL )
        return;
    setColorChooseNotifyHandler(onShapeColorChosen);
    menuBld = gtk_builder_new_from_resource(
            "/org/rafaello7/wilqpaint/menubar.ui");
    menuBar = G_MENU_MODEL(gtk_builder_get_object(menuBld, "menuBar"));
    gtk_application_set_menubar(app, menuBar);
    g_object_unref(menuBld);

    builder = gtk_builder_new_from_resource(
            "/org/rafaello7/wilqpaint/wilqpaint.ui");
    gtk_builder_connect_signals(builder, NULL);
    window = GTK_WIDGET(gtk_builder_get_object(builder, "mainWindow"));
    g_action_map_add_action_entries(G_ACTION_MAP(window),
            win_entries, G_N_ELEMENTS(win_entries), window);

    if( fname == NULL ) {
        drawing = GTK_WIDGET(gtk_builder_get_object(builder, "drawing"));
        g_object_get(G_OBJECT(drawing), "width-request", &imgWidth,
                "height-request", &imgHeight, NULL);
    }else{
        imgWidth = imgHeight = 0;
    }
    openFile(fname, imgWidth, imgHeight);
    gtk_application_add_window(app, GTK_WINDOW(window));
    gtk_widget_show_all(window);
}

static void on_app_activate(GtkApplication* app, gpointer user_data)
{
    initializeApp(app, NULL);
}

void on_app_open(GtkApplication *app, gpointer files,
               gint n_files, gchar *hint, gpointer user_data)
{
    GFile **gfiles = files;
    char *fname = g_file_get_path(gfiles[0]);

    initializeApp(app, fname);
    g_free(fname);
}

int main(int argc, char *argv[])
{
    GtkApplication *app;
    int status;

    app = gtk_application_new (NULL, G_APPLICATION_HANDLES_OPEN);
    g_signal_connect (app, "activate", G_CALLBACK (on_app_activate), NULL);
    g_signal_connect (app, "open", G_CALLBACK (on_app_open), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);
    if( builder != NULL )
        g_object_unref(G_OBJECT(builder));
    return status;
}
