# feh
Imlib2 based image viewer
---

 * http://feh.finalrewind.org/
 * http://linuxbrit.co.uk/feh/
 * #feh on irc.oftc.net

Dependencies
---

 * Imlib2
 * libcurl (disable with make curl=0)
 * libpng
 * libX11
 * libXinerama (disable with make xinerama=0)

If built with exif=1:

 * libexif-dev
 * libexif12

Recommended
---

 * jpegtran  (supplied by the jpeg library, for lossless image rotation)
 * convert  (supplied by ImageMagick, can be used to load unsupported formats)

Installation
---

**For end users:**
```bash
$ make
$ sudo make install app=1
```

**For package maintainers and users who do not want feh to install its
icons into /usr/share:**
```bash
$ make
$ sudo make install
```

**Explanation:** feh ships some icons and an X11 desktop entry, which allow it to
be used from file managers, desktop menus and similar. However, installing
icons to /usr/local/share/... does not seem to work reliable in all cases.
Because of this, when using "make install app=1", feh will install its icons
to /usr/share/..., even though they technically belong into /usr/local.


ZSH Completion for feh is available [here](http://git.finalrewind.org/zsh/plain/etc/completions/_feh)

Make flags
----------

Flags can be used to control the build and installation process.

e.g.

```bash
make flag=bool
```
```bash
make install flag=bool
```
or
```bash
export flag=bool
make && make install
```

For example, `make xinerama=0 debug=1` will disable Xinerama support and produce a debug build.

Available flags are:

| Flag | Default value | Description |
| :--- | :---: | :--- |
| app  | 0 | install icons to /usr/share, regardless of `DESTDIR` and `PREFIX, and call gtk-update-icon-cache afterwards |
| cam  | 0 | install deprecated feh-cam und gen-cam-menu scripts |
| curl | 1 | use libcurl to view http:// and similar images |
| debug | 0 | debug build, enables `--debug` |
| exif | 0 | Builtin EXIF tag display support |
| help | 0 | include help text (refers to the manpage otherwise) |
| stat64 | 0 | Support CIFS shares from 64bit hosts on 32bit machines |
| xinerama | 1 | Support Xinerama/XRandR multiscreen setups |

So, by default **libcurl** and **Xinerama** are enabled, the rest is disabled.

Additionally, the standard variables `PREFIX` and `DESTDIR` are supported.

**PREFIX _(default: /usr)_** controls where the application and its data files
will be installed. It must be set both during `make` and `make install`.

**DESTDIR _(default: empty)_** sets the installation root during "make install". It
is mostly useful for package maintainers.

**Note:** config.mk is designed so that in most cases, you can set environment
variables instead of editing it. E.g.:

```bash
CFLAGS='-g -Os' make
```
```bash
export DESTDIR=/tmp/feh PREFIX=/usr
make && make install
```

Builtin EXIF support is maintained by Dennis Real, [here](https://github.com/reald/feh).


Testing (non-X)
---------------
```bash
$ make test
```

Requires **perl >= 5.10** with `Test::Command`. The tests are non-interactive and
work without X, so they can safely be run even on a headless buildserver.


Testing (X)
-----------

Requires
 * import (usually supplied by imagemagick)
 * perl >= 5.10 with GD, Test::More and X11::GUITest
 * twm
 * Xephyr

```bash
$ make test-x11
```

**_Be aware that this is quite experimental, so far the X-tests have only been
run on one machine. So they may or may not work for you._**
