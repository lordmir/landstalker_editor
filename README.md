# Landstalker Editor

An editor for modifying disassemblies of Landstalker ROMs.

# Linux

## Packages

In addition to build-essential (compiler and make), the following libraries are required:
- wxWidgets
- wine (for running the 68k assembler)

### Ubuntu / Debian

```sh
 # apt install wx-common libwxbase3.0-dev libwxgtk3.0-dev wine-stable
```

### Arch Linux

Firstly, [enable the multilib repositories](https://wiki.archlinux.org/index.php/Official_repositories#multilib)

```sh
 # pacman -S wxgtk3 wine
 # ln -s /usr/bin/wx-config-gtk3 /usr/bin/wx-config
```

## Build

### With make

 build using the Makefile by calling make

```sh
 $ make
```

To build with debug symbols, use pass the DEBUG=yes parameter to make

```sh
 $ make DEBUG=yes
```

# Windows

## Visual Studio 2019 (Community)

### Prerequisites

#### WxWidgets

You will need to download and build the WxWidgets 3.1.3 library (https://www.wxwidgets.org/). Download the Windows ZIP version and extract somewhere convenient (e.g. the root of the C: drive). Note that the final extracted path cannot contain spaces.

Navigate to the extracted WxWidgets sources and go to the build\msw directory. Open the wx_vc16.sln solution file. Go to the "Build" menu and select "Batch Build...". In the batch build window, click the "Select All" button and then click "Build" (note that this will take some time). Once all files have been built, exit out of Visual Studio.

Finally, we need to add an environment variable to tell Visual Studio where to find the WxWidgets files. Open the Start menu, right click on "Computer" and select "Properties". Click on "Advanced System Settings" and then "Environment Variables". Add a new System Environment Variable named WX_WIN, and set its value equal to the full path to the WxWidgets directory (e.g. "C:\libraries\WxWidgets-3.1.3"). Click OK and exit out of the system properties windows.

Make sure that Visual Studio has been restarted so that it picks up the new environment variable.

### Build

Open the VS2019 solution file in landstalker_gfx\VC2019. Make sure that the appropriate configuration has been set (e.g. "Debug x86") and hit Ctrl+Shift+B to build.
