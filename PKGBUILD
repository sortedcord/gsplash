pkgname=gsplash-git
pkgver=0.1.0
pkgrel=1
pkgdesc="A lightweight native SDL2 splash screen wrapper for game launchers."
arch=('x86_64')
depends=('sdl2' 'sdl2_image')
makedepends=('make' 'gcc' 'pkgconf')

# Leave this empty so makepkg doesn't look for external downloads or local copies
source=()
sha256sums=()

build() {
  # Point directly to your active project folder where the Makefile lives
  cd "$startdir"
  make
}

package() {
  cd "$startdir"
  make DESTDIR="$pkgdir" PREFIX="/usr" install
}