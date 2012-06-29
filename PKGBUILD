# Contributor: Sean Pringle <sean.pringle@gmail.com>

pkgname=simpleswitcher-git
pkgver=20120625
pkgrel=1
pkgdesc="simple EWMH window switcher"
arch=('i686' 'x86_64')
url="http://github.com/seanpringle/simpleswitcher"
license=('MIT')
depends=('libx11' 'libxft' 'freetype2')
makedepends=('git')
provides=('simpleswitcher')

_gitroot="git://github.com/seanpringle/simpleswitcher.git"
_gitname="simpleswitcher"

build() {
  cd "$srcdir"
  msg "Connecting to GIT server...."

  if [ -d $_gitname ] ; then
    cd $_gitname && git pull origin
    msg "The local files are updated."
  else
    git clone $_gitroot --depth=1
  fi

  msg "GIT checkout done or server timeout"
  msg "Starting make..."

  rm -rf "$srcdir/$_gitname-build"
  cp -r "$srcdir/$_gitname" "$srcdir/$_gitname-build"
  cd "$srcdir/$_gitname-build"

  make
}

package() {
  cd "$srcdir/$_gitname-build"
  install -Dm 755 $_gitname "$pkgdir/usr/bin/simpleswitcher"
  gzip -c "$_gitname.1" > "$_gitname.1.gz"
  install -Dm644 "$_gitname.1.gz" "$pkgdir/usr/share/man/man1/$_gitname.1.gz"
}