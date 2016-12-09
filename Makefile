OBJS = wlqpersistence.o hittest.o shapedrawing.o shape.o \
	   drawimage.o colorchooser.o wilqpaint.o sizedialog.o \
	   griddialog.o quitdialog.o aboutdialog.o resources.o

UI =  wilqpaint.ui sizedialog.ui griddialog.ui quitdialog.ui aboutdialog.ui \
	  menubar.ui

IMG = img/area.png img/arrow.png        img/free.png      img/imagesize.png \
	  img/line.png img/oval.png         img/rectangle.png img/select.png \
	  img/text.png img/transparency.png img/triangle.png  img/wilqpaint.png \
	  img/pin_left.png img/pin_right.png img/pin_top.png img/pin_bottom.png \
	  img/pin_center.png

wilqpaint: $(OBJS)
	gcc $(OBJS) -o wilqpaint $$(pkg-config --libs gtk+-3.0) -lm -rdynamic
	
.c.o:
	gcc -g -c $$(pkg-config --cflags gtk+-3.0) $<


resources.c: wilqpaint.gresource.xml $(UI) $(IMG)
	glib-compile-resources $< --generate-source --target=$@

clean:
	rm -f $(OBJS) wilqpaint resources.c

tar:
	cd .. && tar cf wilqpaint/wilqpaint.tar.gz  wilqpaint/*.[ch] wilqpaint/*.ui wilqpaint/img wilqpaint/wilqpaint.gresource.xml wilqpaint/Makefile
