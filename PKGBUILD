# Maintainer: Rafal <fatwildcat@gmail.com>
pkgname=wilqpaint
pkgver=0.1
pkgrel=1
pkgdesc="Image painting program"
arch=('x86_64')
url="https://github.com/rafaello7/wilqpaint"
license=('GPL')
depends=('gtk3')
source=("$pkgname-$pkgver.tar.gz")
md5sums=('SKIP')

build() {
	cd "$pkgname-$pkgver"
	./configure --prefix=/usr
	make
}

package() {
	cd "$pkgname-$pkgver"
	make DESTDIR="$pkgdir/" install
}
