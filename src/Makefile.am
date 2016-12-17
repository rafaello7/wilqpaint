SUBDIRS = images

UI =  wilqpaint.ui sizedialog.ui griddialog.ui quitdialog.ui aboutdialog.ui \
	  menubar.ui opendialog.ui savedialog.ui

IMG = images/area.png images/arrow.png images/free.png images/imagesize.png \
	  images/line.png images/oval.png  images/rectangle.png images/select.png \
	  images/text.png images/transparency.png images/triangle.png \
	  images/pin_left.png images/pin_right.png images/pin_top.png \
	  images/pin_bottom.png images/pin_center.png \
	  images/wilqpaint.svg 

bin_PROGRAMS = wilqpaint

wilqpaint_SOURCES = wlqpersistence.c hittest.c shapedrawing.c \
					shape.c drawimage.c colorchooser.c opendialog.c \
					savedialog.c sizedialog.c griddialog.c quitdialog.c \
					aboutdialog.c imgtype.c imagefile.c wilqpaintwin.c \
					wilqpaintapp.c wilqpaint.c \
					aboutdialog.h colorchooser.h drawimage.h griddialog.h \
					hittest.h imgtype.h imagefile.h opendialog.h \
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

