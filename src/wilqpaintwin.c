#include <gtk/gtk.h>
#include "wilqpaintwin.h"
#include "drawimage.h"
#include "colorchooser.h"
#include "griddialog.h"
#include "opendialog.h"
#include "savedialog.h"
#include "quitdialog.h"
#include "sizedialog.h"
#include "thresholddialog.h"
#include "aboutdialog.h"
#include "imgtype.h"
#include "imagefile.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

struct WilqpaintWindow {
    GtkApplicationWindow parent;
};

struct WilqpaintWindowClass {
    GtkApplicationWindowClass parent_class;
};

typedef enum {
    MA_NONE,        /* mouse button is up */
    MA_LAYOUT,
    MA_SELECTAREA,
    MA_MOVEIMAGE
} MouseAction;

typedef struct {
    char *curFileName;
    DrawImage *drawImage;
    MouseAction curAction;

    /* MA_LAYOUT */
    gdouble lastEvX, lastEvY;

    /* selection - MA_SELECTAREA */
    gdouble selXref, selYref, selWidth, selHeight;

    /* MA_MOVEIMAGE */
    gdouble moveXref, moveYref;

    gdouble curZoom;
    gint shapeControlsSetInProgress;
    GridOptions *gopts;

    /* controls */
    GtkWidget       *drawing;
    GtkStack        *shapeTools;
    GtkSpinButton   *thickness;
    GtkSpinButton   *angle;
    GtkSpinButton   *round;
    GtkToggleButton *drawBottom;
    GtkColorChooser *strokeColor;
    GtkColorChooser *fillColor;
    GtkColorChooser *textColor;
    GtkColorChooser *backgroundColor;
    GtkTextView     *textView;
    GtkFontButton   *fontButton;
    GtkWidget       *shapePreview;

    GtkToggleButton *shapeSelect;
    GtkToggleButton *shapeImageSize;
    GtkToggleButton *shapeText;
    GtkToggleButton *shapeFreeForm;
    GtkToggleButton *shapeLine;
    GtkToggleButton *shapeArrow;
    GtkToggleButton *shapeTriangle;
    GtkToggleButton *shapeRect;
    GtkToggleButton *shapeOval;

    GtkSpinButton   *spinImageWidth;
    GtkSpinButton   *spinImageHeight;
    GtkToggleButton *marginLeft;
    GtkToggleButton *marginRight;
    GtkToggleButton *marginTop;
    GtkToggleButton *marginBottom;
} WilqpaintWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(WilqpaintWindow, wilqpaint_window,
        GTK_TYPE_APPLICATION_WINDOW);

static void wilqpaint_window_init(WilqpaintWindow *win)
{
    WilqpaintWindowPrivate *priv;

    gtk_widget_init_template(GTK_WIDGET(win));
    priv = wilqpaint_window_get_instance_private(win);
    priv->curFileName = NULL;
    priv->drawImage = NULL;
    priv->curAction = MA_NONE;
    priv->moveXref = 0;
    priv->moveYref = 0;
    priv->selWidth = 0;
    priv->selHeight = 0;
    priv->curZoom = 1.0;
    priv->shapeControlsSetInProgress = 0;
    priv->gopts = grid_optsNew();
}

static void wilqpaint_window_dispose(GObject *object)
{
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    /* note: wilqpaint_window_dispose may be invoked more than once
     * for the same object
     */
    win = WILQPAINT_WINDOW(object);
    priv = wilqpaint_window_get_instance_private(win);
    grid_optsFree(priv->gopts);
    priv->gopts = NULL;
    G_OBJECT_CLASS(wilqpaint_window_parent_class)->dispose(object);
}

static void onShapeColorChosen(GtkWidget*, enum ChosenColor);

static void wilqpaint_window_class_init(WilqpaintWindowClass *wilqpaintClass)
{
    GtkWidgetClass *widgetClass = GTK_WIDGET_CLASS(wilqpaintClass);
    G_OBJECT_CLASS(wilqpaintClass)->dispose = wilqpaint_window_dispose;
    gtk_widget_class_set_template_from_resource(widgetClass,
            "/org/rafaello7/wilqpaint/wilqpaint.ui");
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, drawing);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, shapeTools);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, thickness);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, angle);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, round);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, drawBottom);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, strokeColor);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, fillColor);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, textColor);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, backgroundColor);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, textView);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, fontButton);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, shapePreview);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, shapeSelect);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, shapeImageSize);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, shapeText);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, shapeFreeForm);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, shapeLine);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, shapeArrow);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, shapeTriangle);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, shapeRect);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, shapeOval);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, spinImageWidth);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, spinImageHeight);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, marginLeft);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, marginRight);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, marginTop);
    gtk_widget_class_bind_template_child_private(widgetClass,
            WilqpaintWindow, marginBottom);
    setColorChooseNotifyHandler(onShapeColorChosen);
}

static void setCurFileName(WilqpaintWindow *win, const char *fname)
{
    char title[128];
    const char *baseName;
    int len;
    WilqpaintWindowPrivate *priv;

    priv = wilqpaint_window_get_instance_private(win);
    g_free(priv->curFileName);
    priv->curFileName = fname == NULL ? NULL : g_strdup(fname);
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
    gtk_window_set_title(GTK_WINDOW(win), title);
}

static void redrawDrawingArea(GtkWidget *drawing)
{
    gdk_window_invalidate_rect(gtk_widget_get_window(drawing), NULL, FALSE);
}

void adjustDrawingSize(WilqpaintWindowPrivate *priv,
        gboolean adjustImageSizeSpins)
{
    gint imgWidth, imgHeight;

    priv->selWidth = priv->selHeight = 0;
    imgWidth = di_getWidth(priv->drawImage);
    imgHeight = di_getHeight(priv->drawImage);
    gtk_widget_set_size_request(priv->drawing, imgWidth * priv->curZoom,
            imgHeight * priv->curZoom);
    if( adjustImageSizeSpins ) {
        ++priv->shapeControlsSetInProgress;
        gtk_spin_button_set_value(priv->spinImageWidth, imgWidth);
        gtk_spin_button_set_value(priv->spinImageHeight, imgHeight);
        --priv->shapeControlsSetInProgress;
    }
}

static void adjustBackgroundColorControl(WilqpaintWindowPrivate *priv)
{
    GdkRGBA color;

    di_getBackgroundColor(priv->drawImage, &color);
    gtk_color_chooser_set_rgba(priv->backgroundColor, &color);
}

static void setControlsFromShapeParams(WilqpaintWindowPrivate *priv,
        const ShapeParams *shapeParams)
{
    GtkTextBuffer *textBuffer;

    ++priv->shapeControlsSetInProgress;
    gtk_color_chooser_set_rgba(priv->strokeColor, &shapeParams->strokeColor);
    gtk_color_chooser_set_rgba(priv->fillColor, &shapeParams->fillColor);
    gtk_color_chooser_set_rgba(priv->textColor, &shapeParams->textColor);
    gtk_spin_button_set_value(priv->thickness, shapeParams->thickness);
    gtk_spin_button_set_value(priv->angle, shapeParams->angle);
    gtk_spin_button_set_value(priv->round, shapeParams->round);
    textBuffer = gtk_text_view_get_buffer(priv->textView);
    if( shapeParams->text ) {
        gtk_text_buffer_set_text(textBuffer, shapeParams->text, -1);
        gtk_font_button_set_font_name(priv->fontButton, shapeParams->fontName);
    }else{
        gtk_text_buffer_set_text(textBuffer, "", 0);
    }
    --priv->shapeControlsSetInProgress;
}

static void getShapeParamsFromControls(WilqpaintWindowPrivate *priv,
        ShapeParams *shapeParams)
{
    GtkTextBuffer *textBuffer;
    GtkTextIter start, end;

    gtk_color_chooser_get_rgba(priv->strokeColor, &shapeParams->strokeColor);
    gtk_color_chooser_get_rgba(priv->fillColor, &shapeParams->fillColor);
    gtk_color_chooser_get_rgba(priv->textColor, &shapeParams->textColor);
    shapeParams->thickness = gtk_spin_button_get_value(priv->thickness);
    shapeParams->angle = gtk_spin_button_get_value(priv->angle);
    shapeParams->round = gtk_spin_button_get_value(priv->round);
    textBuffer = gtk_text_view_get_buffer(priv->textView);
    gtk_text_buffer_get_bounds (textBuffer, &start, &end);
    shapeParams->text =
        gtk_text_buffer_get_text(textBuffer, &start, &end, FALSE);
    shapeParams->fontName = gtk_font_button_get_font_name(priv->fontButton);
}

void on_shapePreview_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    ShapeParams shapeParams;
    Shape *shape = NULL;
    int i;
    gdouble thickness, round, winWidth, winHeight, shapeWidth, shapeHeight;
    gdouble shapeMargin;
    gboolean hasText;
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(widget));
    priv = wilqpaint_window_get_instance_private(win);
    winWidth = gtk_widget_get_allocated_width(widget);
    winHeight = gtk_widget_get_allocated_height(widget);
    if( winWidth <= 20 || winHeight <= 20 )
        return;
    getShapeParamsFromControls(priv, &shapeParams);
    hasText = shapeParams.text && shapeParams.text[0];
    g_free((void*)shapeParams.text);
    shapeParams.text = hasText ? "Ww" : NULL;
    thickness = shapeParams.thickness;
    round = shapeParams.round;
    if( gtk_toggle_button_get_active(priv->shapeFreeForm) ) {
        shape = shape_new(ST_FREEFORM, 0, winHeight / 6, &shapeParams);
        for(i = 4; i < winWidth; i += 4) {
            shape_layoutNew(shape, i, winHeight * (1.0/6.0
                        + 0.2 * (3.0 - cos(53.0 * G_PI / winWidth)
                            - 4.0 * fabs(i - 0.5 * winWidth) / winWidth
                            - cos(5.3 * (i - 10) / winWidth * G_PI))), FALSE);
        }
    }else if( gtk_toggle_button_get_active(priv->shapeLine) ) {
        shapeMargin = 2;
        shape = shape_new(ST_LINE, shapeMargin, 0.5 * winHeight, &shapeParams);
        shape_layoutNew(shape, winWidth - shapeMargin, 0.5 * winHeight, FALSE);
    }else if( gtk_toggle_button_get_active(priv->shapeArrow) ) {
        shapeMargin = 2;
        shape = shape_new(ST_ARROW, shapeMargin, 0.5 * winHeight, &shapeParams);
        shape_layoutNew(shape, winWidth - shapeMargin, 0.5 * winHeight, FALSE);
    }else if( gtk_toggle_button_get_active(priv->shapeTriangle) ) {
        gdouble htop, hbottom, angle = shapeParams.angle * G_PI / 360;
        if( round == 0 ) {
            /* it looks that cairo_stroke cutts vertex of triangle when
             * angle is less than 12 degrees */
            if( shapeParams.angle >= 12 )
                htop = 0.5 * thickness / sin(angle);
            else
                htop = 0.5 * thickness;
            hbottom = 0.5 * thickness;
            gdouble hh = winHeight - htop - hbottom - 8;
            gdouble hw = 0.5 * (winWidth - 8) / tan(angle);
            /* 12 degrees boundary of opposite angle */
            if( shapeParams.angle <= 180 - 2 * 12 )
                hw -= 0.5 * thickness * (1.0 + 1.0 / sin(angle));
            else
                hw -= 0.5 * thickness / tan(angle);
            shapeHeight = fmax(1, fmin(hh, hw));
        }else{
            htop = 0.5 * thickness;
            hbottom = 0.5 * thickness;
            gdouble hh = winHeight - htop - hbottom - 8;
            gdouble hw = (0.5 * (winWidth - 8 - thickness) - round)/tan(angle)
                + 2 * round;
            shapeHeight = fmax(1, fmin(hh, hw));
        }
        shape = shape_new(ST_TRIANGLE, 0.5 * winWidth,
                0.5 * (winHeight - shapeHeight + htop - hbottom),
                &shapeParams);
        shape_layoutNew(shape, 0.5 * winWidth,
                0.5 * (winHeight + shapeHeight + htop - hbottom), FALSE);
    }else if( gtk_toggle_button_get_active(priv->shapeRect) ) {
        shapeWidth = winWidth - 16 - thickness;
        shapeHeight = winHeight - 16 - thickness;
        round = fmin(round, 0.5 * fmin(shapeWidth, shapeHeight));
        shapeWidth = fmax(1, shapeWidth - round * (2 - G_SQRT2));
        shapeHeight = fmax(1, shapeHeight - round * (2 - G_SQRT2));
        shape = shape_new(ST_RECT, 0.5 * (winWidth - shapeWidth),
                0.5 * (winHeight - shapeHeight), &shapeParams);
        shape_layoutNew(shape, 0.5 * (winWidth + shapeWidth),
                0.5 * (winHeight + shapeHeight), FALSE);
    }else if( gtk_toggle_button_get_active(priv->shapeOval) ) {
        shapeWidth = fmax(1, 0.5 * G_SQRT2 * (winWidth - thickness - 8));
        shapeHeight = fmax(1, 0.5 * G_SQRT2 * (winHeight - thickness - 8));
        shape = shape_new(ST_OVAL, 0.5 * (winWidth - shapeWidth),
                0.5 * (winHeight - shapeHeight), &shapeParams);
        shape_layoutNew(shape,  0.5 * (winWidth + shapeWidth),
                0.5 * (winHeight + shapeHeight), FALSE);
    }else if( gtk_toggle_button_get_active(priv->shapeText) ) {
        shapeParams.text = "Ww";
        shape = shape_new(ST_TEXT, 0.5 * winWidth, 0.5 * winHeight,
                &shapeParams);
        shape_layoutNew(shape, 0.5 * winWidth, 0.5 * winHeight, FALSE);
    }
    if( shape != NULL ) {
        shape_draw(shape, cr, 1.0, FALSE, FALSE);
        shape_unref(shape);
    }
}

void on_thickness_value_changed(GtkSpinButton *spin, gpointer user_data)
{
    ShapeParams shapeParams;
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(spin)));
    priv = wilqpaint_window_get_instance_private(win);
    if( priv->shapeControlsSetInProgress == 0 ) {
        shapeParams.thickness = gtk_spin_button_get_value(spin);
        di_setSelectionParam(priv->drawImage, SP_THICKNESS, &shapeParams);
        redrawDrawingArea(priv->drawing);
        redrawDrawingArea(priv->shapePreview);
    }
}

void on_round_value_changed(GtkSpinButton *spin, gpointer user_data)
{
    ShapeParams shapeParams;
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(spin)));
    priv = wilqpaint_window_get_instance_private(win);
    if( priv->shapeControlsSetInProgress == 0 ) {
        shapeParams.round = gtk_spin_button_get_value(spin);
        di_setSelectionParam(priv->drawImage, SP_ROUND, &shapeParams);
        redrawDrawingArea(priv->drawing);
        redrawDrawingArea(priv->shapePreview);
    }
}

void on_angle_value_changed(GtkSpinButton *spin, gpointer user_data)
{
    ShapeParams shapeParams;
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(spin)));
    priv = wilqpaint_window_get_instance_private(win);
    if( priv->shapeControlsSetInProgress == 0 ) {
        shapeParams.angle = gtk_spin_button_get_value(spin);
        di_setSelectionParam(priv->drawImage, SP_ANGLE, &shapeParams);
        redrawDrawingArea(priv->drawing);
        redrawDrawingArea(priv->shapePreview);
    }
}

void on_shapeTextBuffer_changed(GtkTextBuffer *textBuffer, gpointer user_data)
{
    ShapeParams shapeParams;
    GtkTextIter start, end;
    WilqpaintWindow *win = user_data;
    WilqpaintWindowPrivate *priv;

    priv = wilqpaint_window_get_instance_private(win);
    if( priv->shapeControlsSetInProgress == 0 ) {
        gtk_text_buffer_get_bounds (textBuffer, &start, &end);
        shapeParams.text =
            gtk_text_buffer_get_text(textBuffer, &start, &end, FALSE);
        shapeParams.fontName = gtk_font_button_get_font_name(priv->fontButton);
        di_setSelectionParam(priv->drawImage, SP_TEXT, &shapeParams);
        g_free((char*)shapeParams.text);
        redrawDrawingArea(priv->drawing);
        redrawDrawingArea(priv->shapePreview);
    }
}

void on_font_font_set(GtkFontButton *fontButton, gpointer user_data)
{
    ShapeParams shapeParams;
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(fontButton)));
    priv = wilqpaint_window_get_instance_private(win);
    if( priv->shapeControlsSetInProgress == 0 ) {
        shapeParams.fontName = gtk_font_button_get_font_name(fontButton);
        di_setSelectionParam(priv->drawImage, SP_FONTNAME, &shapeParams);
        redrawDrawingArea(priv->drawing);
    }
}

void on_textColor_color_set(GtkColorButton *colorButton, gpointer user_data)
{
    ShapeParams shapeParams;
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(colorButton)));
    priv = wilqpaint_window_get_instance_private(win);
    if( priv->shapeControlsSetInProgress == 0 ) {
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(colorButton),
                &shapeParams.textColor);
        di_setSelectionParam(priv->drawImage, SP_TEXTCOLOR, &shapeParams);
        redrawDrawingArea(priv->drawing);
    }
}

void on_strokeColor_color_set(GtkColorButton *colorButton, gpointer user_data)
{
    ShapeParams shapeParams;
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(colorButton)));
    priv = wilqpaint_window_get_instance_private(win);
    if( priv->shapeControlsSetInProgress == 0 ) {
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(colorButton),
                &shapeParams.textColor);
        di_setSelectionParam(priv->drawImage, SP_STROKECOLOR, &shapeParams);
        redrawDrawingArea(priv->drawing);
        redrawDrawingArea(priv->shapePreview);
    }
}

void on_fillColor_color_set(GtkColorButton *colorButton, gpointer user_data)
{
    ShapeParams shapeParams;
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(colorButton)));
    priv = wilqpaint_window_get_instance_private(win);
    if( priv->shapeControlsSetInProgress == 0 ) {
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(colorButton),
                &shapeParams.textColor);
        di_setSelectionParam(priv->drawImage, SP_FILLCOLOR, &shapeParams);
        redrawDrawingArea(priv->drawing);
        redrawDrawingArea(priv->shapePreview);
    }
}

static void onShapeColorChosen(GtkWidget *widget, enum ChosenColor cc)
{
    ShapeParams shapeParams;
    GdkRGBA color;
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(widget)));
    priv = wilqpaint_window_get_instance_private(win);
    gtk_color_chooser_get_rgba(cc == CC_STROKE ? priv->strokeColor
            : priv->fillColor, &color);
    if( cc == CC_STROKE ) {
        shapeParams.strokeColor = color;
        di_setSelectionParam(priv->drawImage, SP_STROKECOLOR, &shapeParams);
    }else{
        shapeParams.fillColor = color;
        di_setSelectionParam(priv->drawImage, SP_FILLCOLOR, &shapeParams);
    }
    redrawDrawingArea(priv->drawing);
    redrawDrawingArea(priv->shapePreview);
}

static void setZoom1x(WilqpaintWindow *win)
{
    GAction *zoomAction;
    GVariant *state;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(win)));
    priv = wilqpaint_window_get_instance_private(win);
    zoomAction = g_action_map_lookup_action(G_ACTION_MAP(win),
            "image-zoom");
    state = g_variant_new_string("1");
    g_action_change_state(zoomAction, state);
    priv->curZoom = 1.0;
}

static gdouble snapXValue(WilqpaintWindowPrivate *priv, gdouble val)
{
    return grid_isSnapTo(priv->gopts) ?
        grid_getSnapXValue(priv->gopts, val) : val;
}

static gdouble snapYValue(WilqpaintWindowPrivate *priv, gdouble val)
{
    return grid_isSnapTo(priv->gopts) ?
        grid_getSnapYValue(priv->gopts, val) : val;
}

gboolean on_drawing_button_press(GtkWidget *widget, GdkEventButton *event,
               gpointer user_data)
{
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;
    gdouble evX, evY;
    ShapeType shapeType;
    ShapeParams shapeParams;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(widget)));
    priv = wilqpaint_window_get_instance_private(win);
    if( event->button == GDK_BUTTON_PRIMARY ) {
        priv->curAction = MA_NONE;
        priv->selWidth = priv->selHeight = 0;
        evX = snapXValue(priv, event->x / priv->curZoom);
        evY = snapYValue(priv, event->y / priv->curZoom);
        if( gtk_toggle_button_get_active(priv->shapeSelect) ) {
            if( event->state & GDK_SHIFT_MASK ) {
                priv->selXref = evX;
                priv->selYref = evY;
                di_selectionFromPoint(priv->drawImage, evX, evY,
                        event->state & GDK_CONTROL_MASK);
                priv->curAction = MA_SELECTAREA;
            }else{
                priv->lastEvX = evX;
                priv->lastEvY = evY;
                if( di_curShapeFromPoint(priv->drawImage, evX, evY,
                            priv->curZoom, event->state & GDK_CONTROL_MASK) )
                {
                    di_getCurShapeParams(priv->drawImage, &shapeParams);
                    setControlsFromShapeParams(priv, &shapeParams);
                    priv->curAction = MA_LAYOUT;
                }
            }
        }else if( gtk_toggle_button_get_active(priv->shapeImageSize) ) {
            priv->moveXref = evX - di_getXRef(priv->drawImage);
            priv->moveYref = evY - di_getYRef(priv->drawImage);
            priv->curAction = MA_MOVEIMAGE;
        }else if( event->state & GDK_CONTROL_MASK
                || ! di_isSelectionEmpty(priv->drawImage) )
        {
            priv->lastEvX = event->x;
            priv->lastEvY = event->x;
            if( di_curShapeFromPoint(priv->drawImage, evX, evY, priv->curZoom,
                        FALSE))
            {
                di_getCurShapeParams(priv->drawImage, &shapeParams);
                setControlsFromShapeParams(priv, &shapeParams);
                priv->curAction = MA_LAYOUT;
            }
        }else{
            if( gtk_toggle_button_get_active(priv->shapeFreeForm) )
                shapeType = ST_FREEFORM;
            else if( gtk_toggle_button_get_active(priv->shapeLine) )
                shapeType = ST_LINE;
            else if( gtk_toggle_button_get_active(priv->shapeTriangle) )
                shapeType = ST_TRIANGLE;
            else if( gtk_toggle_button_get_active(priv->shapeRect) )
                shapeType = ST_RECT;
            else if( gtk_toggle_button_get_active(priv->shapeOval) )
                shapeType = ST_OVAL;
            else if( gtk_toggle_button_get_active(priv->shapeText) )
                shapeType = ST_TEXT;
            else
                shapeType = ST_ARROW;
            getShapeParamsFromControls(priv, &shapeParams);
            di_addShape(priv->drawImage, shapeType, evX, evY, &shapeParams,
                    gtk_toggle_button_get_active(priv->drawBottom));
            g_free((void*)shapeParams.text);
            priv->lastEvX = evX;
            priv->lastEvY = evY;
            priv->curAction = MA_LAYOUT;
        }
        redrawDrawingArea(priv->drawing);
        gtk_widget_grab_focus(widget);
    }
    return GDK_EVENT_STOP;
}

gboolean on_drawing_motion(GtkWidget *widget, GdkEventMotion *event,
               gpointer user_data)
{
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;
    gdouble evX, evY;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(widget)));
    priv = wilqpaint_window_get_instance_private(win);
    evX = snapXValue(priv, event->x / priv->curZoom);
    evY = snapYValue(priv, event->y / priv->curZoom);

    if( event->state & GDK_BUTTON1_MASK ) {
        switch( priv->curAction ) {
        case MA_LAYOUT:
            di_selectionDragTo(priv->drawImage, evX, evY,
                    event->state & GDK_SHIFT_MASK);
            priv->lastEvX = evX;
            priv->lastEvY = evY;
            break;
        case MA_MOVEIMAGE:
            di_moveTo(priv->drawImage, evX - priv->moveXref,
                    evY - priv->moveYref);
            break;
        case MA_SELECTAREA:
            priv->selWidth = evX - priv->selXref;
            priv->selHeight = evY - priv->selYref;
            di_selectionFromRect(priv->drawImage, evX, evY);
            break;
        }
        redrawDrawingArea(priv->drawing);
    }
    return GDK_EVENT_STOP;
}

gboolean on_drawing_button_release(GtkWidget *widget, GdkEventButton *event,
               gpointer user_data)
{
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(widget)));
    priv = wilqpaint_window_get_instance_private(win);
    priv->curAction = MA_NONE;
}

gboolean on_drawing_keypress(GtkWidget *widget, GdkEventKey *event,
        gpointer user_data)
{
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(widget)));
    priv = wilqpaint_window_get_instance_private(win);
    switch( event->keyval ) {
    case GDK_KEY_Delete:
        if( di_selectionDelete(priv->drawImage) )
            redrawDrawingArea(priv->drawing);
        break;
    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
        if( priv->curAction == MA_LAYOUT && event->state & GDK_BUTTON1_MASK ) {
            di_selectionDragTo(priv->drawImage, priv->lastEvX,
                    priv->lastEvY, TRUE);
            redrawDrawingArea(priv->drawing);
        }
        break;
    default:
        break;
    }
    return GDK_EVENT_PROPAGATE;
}

gboolean on_drawing_keyrelease(GtkWidget *widget, GdkEventKey *event,
        gpointer user_data)
{
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(widget)));
    priv = wilqpaint_window_get_instance_private(win);
    switch( event->keyval ) {
    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
        if( priv->curAction == MA_LAYOUT && event->state & GDK_BUTTON1_MASK ) {
            di_selectionDragTo(priv->drawImage, priv->lastEvX,
                    priv->lastEvY, FALSE);
            redrawDrawingArea(priv->drawing);
        }
        break;
    default:
        break;
    }
    return GDK_EVENT_PROPAGATE;
}

gboolean on_drawing_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    gdouble scale, gridXOffset, gridYOffset, dashes[2];
    gint i, imgWidth, imgHeight;
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(widget)));
    priv = wilqpaint_window_get_instance_private(win);

    di_draw(priv->drawImage, cr, priv->curZoom);
    scale = grid_getScale(priv->gopts) * priv->curZoom;
    if( grid_isShow(priv->gopts) && scale > 2 ) {
        gridXOffset = grid_getXOffset(priv->gopts) * priv->curZoom;
        gridYOffset = grid_getYOffset(priv->gopts) * priv->curZoom;
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
        imgWidth = di_getWidth(priv->drawImage);
        imgHeight = di_getHeight(priv->drawImage);
        for(i = gridXOffset; i <= imgWidth * priv->curZoom; i += scale) {
            cairo_move_to(cr, i, 0);
            cairo_line_to(cr, i, imgHeight * priv->curZoom);
            cairo_stroke(cr);
        }
    }
    if( priv->selWidth != 0 || priv->selHeight != 0 ) {
        cairo_rectangle(cr, priv->selXref * priv->curZoom,
                priv->selYref * priv->curZoom,
                priv->selWidth * priv->curZoom,
                priv->selHeight * priv->curZoom);
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
    return GDK_EVENT_STOP;
}

void on_spinImageSize_value_changed(GtkSpinButton *spin, gpointer user_data)
{
    gdouble translateXfactor = 0.5, translateYfactor = 0.5;
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(spin)));
    priv = wilqpaint_window_get_instance_private(win);
    if( priv->shapeControlsSetInProgress == 0 ) {
        if( gtk_toggle_button_get_active(priv->marginLeft) )
            translateXfactor = 1.0;
        else if( gtk_toggle_button_get_active(priv->marginRight) )
            translateXfactor = 0.0;
        else if( gtk_toggle_button_get_active(priv->marginTop) )
            translateYfactor = 1.0;
        else if( gtk_toggle_button_get_active(priv->marginBottom) )
            translateYfactor = 0.0;
        di_setSize(priv->drawImage,
                gtk_spin_button_get_value(priv->spinImageWidth),
                gtk_spin_button_get_value(priv->spinImageHeight),
                translateXfactor, translateYfactor);
        adjustDrawingSize(priv, FALSE);
    }
}

typedef enum {
    STP_SHAPE,
    STP_IMAGESIZE
} ShapeToolsPage;

static void setShapeToolsActivePage(GtkToggleButton *toggle, ShapeToolsPage stp)
{
    const char *pageName;
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(toggle)));
    priv = wilqpaint_window_get_instance_private(win);
    switch( stp ) {
    case STP_IMAGESIZE:
        pageName = "toolPageImageSize";
        break;
    default:
        pageName = "toolPageShape";
        break;
    }
    gtk_stack_set_visible_child_name(priv->shapeTools, pageName);
    if( di_selectionSetEmpty(priv->drawImage)
            || priv->selWidth != 0 || priv->selHeight != 0 )
    {
        priv->selWidth = priv->selHeight = 0;
        redrawDrawingArea(priv->drawing);
    }
    redrawDrawingArea(priv->shapePreview);
}

void on_shapeSelect_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(toggle, STP_SHAPE);
}

void on_shapeImageSize_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(toggle, STP_IMAGESIZE);
}

void on_shapeFreeForm_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(toggle, STP_SHAPE);
}

void on_shapeLine_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(toggle, STP_SHAPE);
}

void on_shapeArrow_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(toggle, STP_SHAPE);
}

void on_shapeText_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(toggle, STP_SHAPE);
}

void on_shapeTriangle_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(toggle, STP_SHAPE);
}

void on_shapeRect_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(toggle, STP_SHAPE);
}

void on_shapeOval_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    if( gtk_toggle_button_get_active(toggle) )
        setShapeToolsActivePage(toggle, STP_SHAPE);
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
static gboolean saveChanges(WilqpaintWindow *win,
        gboolean onQuit, gboolean forceChooseFileName)
{
    WilqpaintWindowPrivate *priv;
    gboolean doSave = TRUE;

    priv = wilqpaint_window_get_instance_private(win);
    if( onQuit ) {
        if( priv->drawImage == NULL || ! di_isModified(priv->drawImage) )
            return TRUE;
        switch( showQuitDialog(GTK_WINDOW(win)) ) {
        case GTK_RESPONSE_YES:
            break;
        case GTK_RESPONSE_CANCEL:
            return FALSE;
        default:
            return TRUE;
        }
    }
    if( priv->curFileName == NULL || forceChooseFileName
            || !imgtype_isWritableByFileName(priv->curFileName) )
    {
        char *fname = showSaveFileDialog(GTK_WINDOW(win), priv->curFileName);
        if( fname ) {
            setCurFileName(win, fname);
            g_free(fname);
        }else
            doSave = FALSE;
    }
    if( doSave ) {
        gchar *err;
        doSave = imgfile_save(priv->drawImage, priv->curFileName, &err);
        if( ! doSave ) {
            GtkWidget *messageDialog = gtk_message_dialog_new(
                    GTK_WINDOW(win), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_CLOSE, "%s", err);
            gtk_dialog_run(GTK_DIALOG(messageDialog));
            gtk_widget_destroy(GTK_WIDGET(messageDialog));
            g_free(err);
        }
    }
    return doSave;
}

static void setCurDrawImage(WilqpaintWindow *win, const char *fileName,
        DrawImage *newDrawImg)
{
    WilqpaintWindowPrivate *priv;

    priv = wilqpaint_window_get_instance_private(win);
    if( priv->drawImage != NULL )
        di_free(priv->drawImage);
    priv->drawImage = newDrawImg;
    setCurFileName(win, fileName);
    setZoom1x(win);
    adjustDrawingSize(priv, TRUE);
    adjustBackgroundColorControl(priv);
}

static void newFile(WilqpaintWindow *win,
        const char *fname, gdouble imgWidth, gdouble imgHeight)
{
    DrawImage *newDrawImg = NULL;
    WilqpaintWindowPrivate *priv;

    priv = wilqpaint_window_get_instance_private(win);
    if( imgWidth < 1.0 || imgHeight < 1.0 ) {
        imgWidth = di_getWidth(priv->drawImage);
        imgHeight = di_getHeight(priv->drawImage);
        if( ! showSizeDialog(GTK_WINDOW(win), "New Image - wilqpaint",
                    &imgWidth, &imgHeight, FALSE) )
            return;
    }
    newDrawImg = di_new(imgWidth, imgHeight, NULL);
    setCurDrawImage(win, fname, newDrawImg);
}

static void openFile(WilqpaintWindow *win, const char *fname)
{
    DrawImage *newDrawImg = NULL;
    WilqpaintWindowPrivate *priv;
    gchar *err;
    gboolean isNoEntErr;

    priv = wilqpaint_window_get_instance_private(win);
    newDrawImg = imgfile_open(fname, &err, &isNoEntErr);
    if( newDrawImg != NULL )
        setCurDrawImage(win, fname, newDrawImg);
    else{
        if( ! isNoEntErr ) {
            GtkWidget *messageDialog = gtk_message_dialog_new(
                    GTK_WINDOW(win), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_CLOSE, "%s", err);
            gtk_dialog_run(GTK_DIALOG(messageDialog));
            gtk_widget_destroy(GTK_WIDGET(messageDialog));
        }
        g_free(err);
    }
}

gboolean on_mainWindow_delete_event(GtkWidget *widget, GdkEvent *event,
        gpointer user_data)
{
    return ! saveChanges(WILQPAINT_WINDOW(widget), TRUE, FALSE);
}

static void on_menu_new(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    if( ! saveChanges(WILQPAINT_WINDOW(window), TRUE, FALSE) )
        return;
    newFile(WILQPAINT_WINDOW(window), NULL, 0, 0);
}

static void on_menu_open(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    char *fname;
    WilqpaintWindowPrivate *priv;

    priv = wilqpaint_window_get_instance_private(WILQPAINT_WINDOW(window));
    if( ! saveChanges(WILQPAINT_WINDOW(window), TRUE, FALSE) )
        return;
    fname = showOpenFileDialog(GTK_WINDOW(window), priv->curFileName);
    if( fname != NULL ) {
        openFile(WILQPAINT_WINDOW(window), fname);
        g_free(fname);
    }
}

static void on_menu_saveas(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    saveChanges(WILQPAINT_WINDOW(window), FALSE, TRUE);
}

static void on_menu_save(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    saveChanges(WILQPAINT_WINDOW(window), FALSE, FALSE);
}

static void on_menu_quit(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    gtk_window_close(GTK_WINDOW(window));
}

static void on_menu_edit_undo(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    WilqpaintWindowPrivate *priv;

    priv = wilqpaint_window_get_instance_private(WILQPAINT_WINDOW(window));
    gint imgWidth = di_getWidth(priv->drawImage);
    gint imgHeight = di_getHeight(priv->drawImage);
    di_undo(priv->drawImage);
    if( di_getWidth(priv->drawImage) != imgWidth
            || di_getHeight(priv->drawImage) != imgHeight )
        adjustDrawingSize(priv, TRUE);
    else
        redrawDrawingArea(priv->drawing);
    adjustBackgroundColorControl(priv);
}

static void on_menu_edit_redo(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    WilqpaintWindowPrivate *priv;
    gint imgWidth, imgHeight;

    priv = wilqpaint_window_get_instance_private(WILQPAINT_WINDOW(window));
    imgWidth = di_getWidth(priv->drawImage);
    imgHeight = di_getHeight(priv->drawImage);
    di_redo(priv->drawImage);
    if( di_getWidth(priv->drawImage) != imgWidth
            || di_getHeight(priv->drawImage) != imgHeight )
        adjustDrawingSize(priv, TRUE);
    else
        redrawDrawingArea(priv->drawing);
    adjustBackgroundColorControl(priv);
}

static void on_menu_image_scale(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    gdouble imgWidth, imgHeight;
    WilqpaintWindowPrivate *priv;

    priv = wilqpaint_window_get_instance_private(WILQPAINT_WINDOW(window));
    imgWidth = di_getWidth(priv->drawImage);
    imgHeight = di_getHeight(priv->drawImage);
    if( showSizeDialog(GTK_WINDOW(window), "Scale image - wilqpaint",
                &imgWidth, &imgHeight, TRUE) )
    {
        di_scale(priv->drawImage, imgWidth / di_getWidth(priv->drawImage));
        adjustDrawingSize(priv, TRUE);
    }
}

static void onThresholdValueChange(GtkWindow *win, gdouble value)
{
    WilqpaintWindowPrivate *priv;

    priv = wilqpaint_window_get_instance_private(WILQPAINT_WINDOW(win));
    di_thresholdPreview(priv->drawImage, value);
    redrawDrawingArea(priv->drawing);
}

static void on_menu_image_threshold(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    gdouble imgWidth, imgHeight;
    WilqpaintWindowPrivate *priv;
    gboolean commit;

    priv = wilqpaint_window_get_instance_private(WILQPAINT_WINDOW(window));
    if( di_hasBaseImage(priv->drawImage) ) {
        di_thresholdPreview(priv->drawImage, 0.5);
        commit = showThresholdDialog(GTK_WINDOW(window), 0.5,
                    onThresholdValueChange);
        di_thresholdFinish(priv->drawImage, commit);
        redrawDrawingArea(priv->drawing);
    }else{
        GtkWidget *messageDialog = gtk_message_dialog_new(
                GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE, "Drawing does not contain base image");
        gtk_dialog_run(GTK_DIALOG(messageDialog));
        gtk_widget_destroy(GTK_WIDGET(messageDialog));
    }
}

void on_backgroundColor_color_set(GtkColorButton *button, gpointer user_data)
{
    GdkRGBA color;
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button)));
    priv = wilqpaint_window_get_instance_private(win);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &color);
    di_setBackgroundColor(priv->drawImage, &color);
    redrawDrawingArea(priv->drawing);
}

static void menu_image_set_zoom(GSimpleAction *action, GVariant *state,
                    gpointer window)
{
    WilqpaintWindowPrivate *priv;

    priv = wilqpaint_window_get_instance_private(WILQPAINT_WINDOW(window));
    priv->curZoom = strtod(g_variant_get_string(state, NULL), NULL);
    g_simple_action_set_state(action, state);
    adjustDrawingSize(priv, TRUE);
}

static void onGridDialogChange(GtkWindow *window)
{
    GAction *action;
    GVariant *state;
    WilqpaintWindow *win;
    WilqpaintWindowPrivate *priv;

    win = WILQPAINT_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(window)));
    priv = wilqpaint_window_get_instance_private(win);
    action = g_action_map_lookup_action(G_ACTION_MAP(win), "grid-show");
    g_action_change_state(action, g_variant_new_boolean(
                grid_isShow(priv->gopts)));
    action = g_action_map_lookup_action(G_ACTION_MAP(win), "grid-snap");
    g_action_change_state(action, g_variant_new_boolean(
                grid_isSnapTo(priv->gopts)));
    redrawDrawingArea(priv->drawing);
}

static void on_menu_grid_options(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    WilqpaintWindowPrivate *priv;

    priv = wilqpaint_window_get_instance_private(WILQPAINT_WINDOW(window));
    grid_showDialog(priv->gopts, GTK_WINDOW(window), onGridDialogChange);
}

static void menu_grid_show (GSimpleAction *action, GVariant *state,
        gpointer window)
{
    WilqpaintWindowPrivate *priv;

    priv = wilqpaint_window_get_instance_private(WILQPAINT_WINDOW(window));
    grid_setIsShow(priv->gopts, g_variant_get_boolean(state));
    g_simple_action_set_state (action, state);
    redrawDrawingArea(priv->drawing);
}

static void menu_grid_snap (GSimpleAction *action, GVariant *state,
        gpointer window)
{
    WilqpaintWindowPrivate *priv;

    priv = wilqpaint_window_get_instance_private(WILQPAINT_WINDOW(window));
    grid_setIsSnapTo(priv->gopts, g_variant_get_boolean (state));
    g_simple_action_set_state(action, state);
}

static void on_menu_about(GSimpleAction *action, GVariant *parameter,
        gpointer window)
{
    showAboutDialog(GTK_WINDOW(window));
}

WilqpaintWindow *wilqpaint_windowNew(GtkApplication *app, const char *fileName)
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
        { "image-threshold", on_menu_image_threshold, NULL, NULL, NULL },
        /* View */
        { "image-zoom", NULL, "s", "'1'", menu_image_set_zoom },
        { "grid-options", on_menu_grid_options, NULL, NULL, NULL },
        { "grid-show", NULL, NULL, "false", menu_grid_show },
        { "grid-snap", NULL, NULL, "false", menu_grid_snap },
        /* Help */
        { "help-about",  on_menu_about,  NULL, NULL, NULL }
    };
    gint imgWidth, imgHeight;
    WilqpaintWindowPrivate *priv;
    GdkPixbuf *icon;

    WilqpaintWindow *mainWin = g_object_new(WILQPAINT_WINDOW_TYPE,
            "application", app, NULL);
    icon = gdk_pixbuf_new_from_resource(
            "/org/rafaello7/wilqpaint/images/wilqpaint.svg", NULL);
    gtk_window_set_icon(GTK_WINDOW(mainWin), icon);
    g_object_unref(icon);
    g_action_map_add_action_entries(G_ACTION_MAP(mainWin),
            win_entries, G_N_ELEMENTS(win_entries), mainWin);

    priv = wilqpaint_window_get_instance_private(mainWin);
    if( fileName != NULL )
        openFile(mainWin, fileName);
    if( priv->drawImage == NULL ) {
        g_object_get(G_OBJECT(priv->drawing), "width-request", &imgWidth,
                "height-request", &imgHeight, NULL);
        newFile(mainWin, fileName, imgWidth, imgHeight);
    }
    gtk_application_add_window(app, GTK_WINDOW(mainWin));
    gtk_widget_show_all(GTK_WIDGET(mainWin));
    return mainWin;
}

