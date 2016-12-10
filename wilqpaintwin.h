#ifndef WILQPAINTWIN_H
#define WILQPAINTWIN_H


#define WILQPAINT_WINDOW_TYPE (wilqpaint_window_get_type ())
#define WILQPAINT_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
            WILQPAINT_WINDOW_TYPE, WilqpaintWindow))


typedef struct WilqpaintWindow         WilqpaintWindow;
typedef struct WilqpaintWindowClass    WilqpaintWindowClass;


GType wilqpaint_window_get_type(void);
WilqpaintWindow *wilqpaint_windowNew(GtkApplication *app, const char *fileName);

#endif /* WILQPAINTWIN_H */
