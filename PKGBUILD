# Maintainer: Peter Drysdale <drysdalepete at gmail com>

pkgname=bellbird-git
pkgver=r263.294ffbe
pkgrel=1
pkgdesc="A speech synthesizer (TTS - text to speech)"
arch=('i686' 'x86_64')
url="https://github.com/peterdrysdale/bellbird"
license=('custom')
depends=('alsa-lib')
makedepends=('git' 'cmake')
options=('strip')
source=('bellbird-git::git+https://github.com/peterdrysdale/bellbird.git')
sha256sums=('SKIP')

pkgver() {
  cd bellbird-git
  printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
  cd bellbird-git
  cmake .
  make
}

package() {
  cd bellbird-git
  install -D -m755 bellbird "${pkgdir}/usr/bin/bellbird"
  install -D -m644 COPYING "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE" 
  install -D -m644 README.md "${pkgdir}/usr/share/doc/${pkgname}/README.md" 
  install -D -m644 doc/dictionary.md "${pkgdir}/usr/share/doc/${pkgname}/dictionary.md" 
  install -D -m644 doc/audiotroubleshooting.md "${pkgdir}/usr/share/doc/${pkgname}/audiotroubleshooting.md" 
}
