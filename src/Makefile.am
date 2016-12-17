UI =  wilqpaint.ui sizedialog.ui griddialog.ui quitdialog.ui aboutdialog.ui \
	  menubar.ui opendialog.ui savedialog.ui

IMG = img/area.png img/arrow.png        img/free.png      img/imagesize.png \
	  img/line.png img/oval.png         img/rectangle.png img/select.png \
	  img/text.png img/transparency.png img/triangle.png  img/wilqpaint.png \
	  img/pin_left.png img/pin_right.png img/pin_top.png img/pin_bottom.png \
	  img/pin_center.png

bin_PROGRAMS = wilqpaint

wilqpaint_SOURCES = wlqpersistence.c hittest.c shapedrawing.c \
					shape.c drawimage.c colorchooser.c opendialog.c \
					savedialog.c sizedialog.c griddialog.c quitdialog.c \
					aboutdialog.c imagetype.c imagefile.c wilqpaintwin.c \
					wilqpaintapp.c wilqpaint.c \
					aboutdialog.h colorchooser.h drawimage.h griddialog.h \
					hittest.h imagetype.h imagefile.h opendialog.h \
					quitdialog.h \
					savedialog.h shapedrawing.h shape.h sizedialog.h \
					wilqpaintapp.h wilqpaintwin.h wlqpersistence.h \
					wilqpaint.gresource.xml \
					$(UI) $(IMG)

nodist_wilqpaint_SOURCES = resources.c

wilqpaint_CFLAGS  = $(LIBGTK_CFLAGS)
wilqpaint_LDADD   = $(LIBGTK_LIBS)
wilqpaint_LDFLAGS = -rdynamic

EXTRA_DIST = wilqpaint.gresource.xml

resources.c: wilqpaint.gresource.xml $(UI) $(IMG)
	glib-compile-resources --sourcedir=$(srcdir) $< --generate-source \
		--target=$@

CLEANFILES = resources.c

