[![build status](https://travis-ci.org/derf/feh.svg?branch=master)](https://travis-ci.org/derf/feh)

Feh – Image viewer and Cataloguer
---

feh is a light-weight, configurable and versatile image viewer.
It is aimed at command line users, but can also be started from graphical file
managers. Apart from viewing images, it can compile text and thumbnail
listings, show (un)loadable files, set X11 backgrounds, and more.

Features include filelists, various image sorting modes, custom action scripts,
and image captions. feh can be controlled by configurable keyboard and mouse
shortcuts, terminal input and signals.  When no file arguments or filelists are
specified, feh displays all files in the current directory.

For more information, please refer to the [feh
website](https://feh.finalrewind.org/) or read the [feh
manual](https://man.finalrewind.org/1/feh/).

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

Recommended Binaries
---

 * convert  (supplied by ImageMagick, can be used to load image formats not supported by Imlib2)

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
icons to /usr/local/share/... does not seem to work reliably.
Because of this, when using "make install app=1", feh will install its icons
to /usr/share/..., even though they technically belong into /usr/local.

[ZSH completion for
feh](https://git.finalrewind.org/zsh/plain/etc/completions/_feh) is also
available.

Make flags
----------

feh's build process uses make flags to enable/disable optional features and
fine-tune the build and installation process. They can be passed as **make**
arguments or set as environment variables, like so:

```bash
make flag=bool
make install flag=bool
```
or
```bash
export flag=bool
make && make install
```

The following flags are respected by the makefile. A default value of **1**
indicates that the corresponding feature is enabled by default.

| Flag | Default value | Description |
| :--- | :---: | :--- |
| app  | 0 | install icons to /usr/share, regardless of `DESTDIR` and `PREFIX`, and call gtk-update-icon-cache afterwards |
| curl | 1 | use libcurl to view https:// and similar images |
| debug | 0 | debug build, enables `--debug` |
| exif | 0 | Builtin EXIF tag display support |
| help | 0 | include help text (refers to the manpage otherwise) |
| inotify | 0 | enable inotify, needed for `--auto-reload` |
| stat64 | 0 | Support CIFS shares from 64bit hosts on 32bit machines |
| verscmp | 1 | Support naturing sorting (`--version-sort`). Requires a GNU-compatible libc exposing `strverscmp` |
| xinerama | 1 | Support Xinerama/XRandR multiscreen setups |

For example, `make xinerama=0 debug=1` will disable Xinerama support and
produce a debug build; libcurl and natural sorting support will remain enabled.

Additionally, the standard variables `PREFIX` and `DESTDIR` are supported.

**PREFIX _(default: /usr/local)_** controls where the application and its data files
will be installed. It must be set both during `make` and `make install`.

**DESTDIR _(default: empty)_** sets the installation root during "make install". It
is mostly useful for package maintainers.

**Note:** Defaults are specified in `config.mk`. It is designed so that in most
cases, you can set environment variables instead of editing it. E.g.:

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

The non-X11 parts of feh can be automatically tested by running

```bash
$ make test
```
This requires **perl >= 5.10** and the perl module `Test::Command`. Tests are
non-interactive and do not require a running X11, so they can safely be run on
a headless buildserver.


Contributing
---

Bugfixes are always welcome, just open a pull request :)

Before proposing a new feature, please consider the scope of feh: It is an
image viewer and cataloguer, not an image editor or similar. Also, its option
list is already pretty long. Please discuss your ideas in a feature request
before opening a pull request in this case.

Please keep in mind that feh's options, key bindings and format specifiers are
documented in two different places: The manual (man/feh.pre) and the help text
(src/help.raw). Although the help is not compiled in by default, it should be
kept up-to-date. On space-constrained embedded systems, it may be more useful
than the (significantly larger) man page.
