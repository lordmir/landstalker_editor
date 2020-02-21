# landstalker_gfx
Landstalker Graphics Viewer

# Linux

## Ubuntu

In addition to build-essential (compiler and make), wxwidget GUI library is required

```sh
 # apt install wx-common libwxbase3.0-dev libwxgtk3.0-dev
```

## Arch Linux

In addition to base-devel (compiler and make), wxwidget GUI library is required

```sh
 # pacman -S wxgtk3
 # ln -s /usr/bin/wx-config-gtk3 /usr/bin/wx-config
```

# Build

## With make

 build using the Makefile by calling make

```sh
 $ make
```

To build with debug symbols, use pass the DEBUG=yes parameter to make

```sh
 $ make DEBUG=yes
```

# Note

* The ROM path cannot contains a folder that start with a dot.

# Screenshot

![edit](landstalker_edit.png)
