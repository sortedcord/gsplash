pkgname=gsplash-git
pkgver=0.1.0
pkgrel=1
pkgdesc="A lightweight SDL2 splash screen wrapper for game launchers."
arch=('x86_64')
depends=('sdl2' 'sdl2_image')
makedepends=('make' 'gcc' 'pkgconf')
source=('src/splash.c' 'Makefile')
sha256sums=('SKIP' 'SKIP')

prepare() {
  mkdir -p "$srcdir/src"
  cp "$srcdir/splash.c" "$srcdir/src/"
}

build() {
  cd "$srcdir"
  make
}

package() {
  cd "$srcdir"
  make DESTDIR="$pkgdir" PREFIX="/usr" install
}
