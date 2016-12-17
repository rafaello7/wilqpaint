#ifndef WILQPAINTAPP_H
#define WILQPAINTAPP_H

#define WILQPAINT_APP_TYPE (wilqpaint_app_get_type())
#define WILQPAINT_APP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
            WILQPAINT_APP_TYPE, WilqpaintApp))

typedef struct WilqpaintApp WilqpaintApp;

GType wilqpaint_app_get_type(void);
WilqpaintApp *wilqpaint_appNew(void);


#endif /* WILQPAINTAPP_H */
