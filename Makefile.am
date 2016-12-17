SUBDIRS = src

desktopdir = $(datadir)/applications
dist_desktop_DATA = wilqpaint.desktop

mimedir = $(datadir)/mime/packages
dist_mime_DATA = wilqpaint.xml

deb: dist
	rm -rf dpkg-deb
	mkdir dpkg-deb
	cd dpkg-deb && tar xf ../$(distdir).tar.gz
	cd dpkg-deb/$(distdir) && dpkg-buildpackage -us -uc

arch: dist
	rm -rf archlinux
	mkdir archlinux
	cp PKGBUILD wilqpaint-*.tar.gz archlinux
	cd archlinux && makepkg
