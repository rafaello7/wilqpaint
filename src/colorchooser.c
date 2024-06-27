#include <gtk/gtk.h>
#include "colorchooser.h"


static GdkPixbuf *transparencyPatt = NULL;
static GdkRGBA curChooseColorStroke = {
    .red = 0.0, .green = 0.0, .blue = 0.0, .alpha = 1.0
};
static GdkRGBA curChooseColorFill = {
    .red = 1.0, .green = 1.0, .blue = 1.0, .alpha = 1.0
};

static const unsigned PRE_COLORS[27] = {
    0000, 0200, 0001, 0201, 0002, 0202, 0012, 0212, 0112,
    0102, 0022, 0101, 0122, 0100, 0121, 0210, 0120, 0211,
    0021, 0221, 0020, 0220, 0011, 0111, 0010, 0222, 0110
};

static const double PRE_ALPHA[6] = { 0.0625, 0.125, 0.25, 0.5, 0.75, 1.0 };
static void (*colorChooseNotifyFun)(GtkWidget*, enum ChosenColor);


gboolean on_colorChooser_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    unsigned i, x = 2, y = 2, preColor;
    GdkRGBA color;
    GtkStyleContext *context;

    if( transparencyPatt == NULL )
        transparencyPatt = gdk_pixbuf_new_from_resource(
                "/org/rafaello7/wilqpaint/images/transparency.png", NULL);
    context = gtk_widget_get_style_context(widget);
    gtk_render_background(context, cr, 0, 0,
            gtk_widget_get_allocated_width(widget),
            gtk_widget_get_allocated_height(widget));
    for(i = 0; i < 40; ++i) {
        if(i == 27 || i > 27 && i != 33 && i != 39 ) {
            cairo_rectangle(cr, x, y, 16, 16);
            gdk_cairo_set_source_pixbuf (cr, transparencyPatt, x, y);
            cairo_fill(cr);
        }
        if( i != 27 ) {
            cairo_rectangle(cr, x, y, 16, 16);
            if( i < 27 ) {
                preColor = PRE_COLORS[i];
                color.red = (preColor >> 6) / 2.0;
                color.green = ((preColor >> 3) & 0x7) / 2.0;
                color.blue = (preColor & 0x7) / 2.0;
                color.alpha = 1.0;
            }else if( i < 34 ) {
                color = curChooseColorStroke;
                color.alpha = 0.25 + PRE_ALPHA[i - 28] * 0.75;
            }else{
                color = curChooseColorFill;
                color.alpha = 0.25 + PRE_ALPHA[i - 34] * 0.75;
            }
            gdk_cairo_set_source_rgba (cr, &color);
            cairo_fill(cr);
        }
        if( y == 20 ) {
            x += 18;
            y = 2;
        }else
            y = 20;
    }
    return FALSE;
}

static void redrawColorChooser(GtkWidget *colorChooser)
{
    GdkRectangle update_rect;
    update_rect.x = 0;
    update_rect.y = 0;
    update_rect.width = gtk_widget_get_allocated_width(colorChooser);
    update_rect.height = gtk_widget_get_allocated_height(colorChooser);
    GdkWindow *gdkWin = gtk_widget_get_window(colorChooser);
    gdk_window_invalidate_rect (gtk_widget_get_window(colorChooser),
            &update_rect, FALSE);
}

gboolean on_colorChooser_button_press(GtkWidget *colorChooser,
        GdkEventButton *event, gpointer user_data)
{
    unsigned itemX, itemY, item, preColor;
    GtkColorChooser *colorBtn;
    GdkRGBA prevBtnColor, newBtnColor;
    GtkGrid *parentGrid;
    enum ChosenColor cc;

    if( (event->button == 1 || event->button == 3)
            && event->x >= 2 && event->y >= 2 )
    {
        cc = event->button == 1 ? CC_STROKE : CC_FILL;
        itemX = ((int)event->x - 2) / 18;
        itemY = ((int)event->y - 2) / 18;
        parentGrid = GTK_GRID(gtk_widget_get_parent(colorChooser));
        colorBtn = GTK_COLOR_CHOOSER(gtk_grid_get_child_at(parentGrid,
                    cc == CC_FILL, 0));
        gtk_color_chooser_get_rgba(colorBtn, &prevBtnColor);
        if( event->x - 18 * itemX < 18 && event->y - 18 * itemY < 18 ) {
            item = 2 * itemX + itemY;
            if( item < 27) {
                preColor = PRE_COLORS[2 * itemX + itemY];
                newBtnColor.red = (preColor >> 6) / 2.0;
                newBtnColor.green = ((preColor >> 3) & 0x7) / 2.0;
                newBtnColor.blue = (preColor & 0x7) / 2.0;
                newBtnColor.alpha = 1.0;
                if( cc == CC_STROKE )
                    curChooseColorStroke = newBtnColor;
                else
                    curChooseColorFill = newBtnColor;
            }else if( item == 27) {
                newBtnColor.red = 0.0;
                newBtnColor.green = 0.0;
                newBtnColor.blue = 0.0;
                newBtnColor.alpha = 0.0;
            }else if( item >= 28 && item < 34 ) {
                newBtnColor = curChooseColorStroke;
                newBtnColor.alpha = PRE_ALPHA[item - 28];
            }else if( item >= 34 && item < 40 ) {
                newBtnColor = curChooseColorFill;
                newBtnColor.alpha = PRE_ALPHA[item - 34];
            }else
                newBtnColor = prevBtnColor;
            if( !gdk_rgba_equal(&prevBtnColor, &newBtnColor) ) {
                gtk_color_chooser_set_rgba(colorBtn, &newBtnColor);
                if( colorChooseNotifyFun )
                    colorChooseNotifyFun(colorChooser, cc);
            }
            redrawColorChooser(colorChooser);
        }
    }
    return FALSE;
}


void setColorChooseNotifyHandler(void (*fun)(GtkWidget*, enum ChosenColor) )
{
    colorChooseNotifyFun = fun;
}

