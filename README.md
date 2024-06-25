# Landstalker Editor
A tool for extracting, viewing, editing and re-inserting the various data found in the Landstalker ROMs and disassembly.

This tool can work with any of the six publicly available ROM dumps, as well as with my other project: [Landstalker Disassembly](https://github.com/lordmir/landstalker_disasm)

Work-in-progress documentation [here](https://github.com/lordmir/landstalker_editor/wiki).

Current State of progress:

| Asset                       | Viewable           | Editable           | Notes                                                      |
|-----------------------------|--------------------|--------------------|------------------------------------------------------------|
| Strings                     | :heavy_check_mark: | :heavy_check_mark: | Custom character sets to be added.                         |
| Script                      | :x:                | :x:                | ROM format reverse engineered.                             |
| Tiles                       | :heavy_check_mark: | :heavy_check_mark: |                                                            |
| 2D Maps                     | :heavy_check_mark: | :heavy_check_mark: |                                                            |
| Misc Graphics (Fonts, etc.) | :heavy_minus_sign: | :heavy_minus_sign: | 90% complete. Still need to understand end credit font.    |
| Palettes                    | :heavy_check_mark: | :heavy_check_mark: |                                                            |
| Blocksets                   | :heavy_check_mark: | :x:                | Editor UI still needs to be made.                          |
| 3D Maps                     | :heavy_check_mark: | :heavy_check_mark: | Editor UI could be better optimised.                       |
| Heightmaps                  | :heavy_check_mark: | :heavy_check_mark: |                                                            |
| Room Entity Placement       | :heavy_check_mark: | :heavy_check_mark: |                                                            |
| Room Warp Placement         | :heavy_check_mark: | :heavy_check_mark: |                                                            |
| Chest Contents              | :heavy_check_mark: | :heavy_check_mark: |                                                            |
| Room Character Scripts      | :heavy_check_mark: | :heavy_check_mark: |                                                            |
| Room Flags                  | :heavy_check_mark: | :heavy_check_mark: |                                                            |
| Room Graphic Transitions    | :heavy_check_mark: | :heavy_check_mark: |                                                            |
| Entity Properties           | :x:                | :x:                | ROM format reverse engineered.                             |
| Entity Behaviours           | :x:                | :x:                | ROM format reverse engineered.                             |
| Sprite Frames               | :heavy_check_mark: | :heavy_minus_sign: | Editor UI could do with improvements.                      |
| Sprite Animations           | :heavy_check_mark: | :x:                |                                                            |

# Build

## Linux

### Packages

In addition to build-essential (compiler and make), the following libraries are required:
- [wxWidgets](https://www.wxwidgets.org/downloads/) (at least v3.2.2)
- libpng
- zlib

It is recommended to have the following installed in addition to the above, in order to build assemblies:
- git
- wine
- kega-fusion

#### Build and Install wxWidgets (all distributions)

 1. Download the wxWidgets v3.2+ source from the [official website](https://www.wxwidgets.org/downloads/). Extract the contents of the tar file to a suitable location.
 2. Install the prerequisites (see the README.md file in the wxWidgets source). For Debian/Ubuntu, you will need to run this command:
    `# apt install libgtk-3-dev libgl1-mesa-dev libglu1-mesa-dev libgstreamer-plugins-base1.0-dev libcurl4-openssl-dev libwebkit2gtk-4.0-dev libpng-dev`
 3. Create a build directory:
    ```sh
    $ cd wxWidgets-3.2.2.1
    $ mkdir gtkbuild
    $ cd gtkbuild
    ```
 4. Run the configure script as follows:
    ```sh
    $ ../configure --with-gtk --disable-shared
    ```
    Add the `--enable-debug` option for a debug build.
 5. Build the source: (this can take some time)
    ```sh
    $ make -j8
    ```
 6. Install the built libraries:
    ```
    sudo make install
    sudo ldconfig
    ```

### Build

 build using the Makefile by calling make

```sh
$ make
```

To build with debug symbols, use pass the DEBUG=yes parameter to make

```sh
$ make DEBUG=yes
```

## Windows

### Visual Studio 2019 (Community)

#### Prerequisites

##### WxWidgets

You will need to download and build the WxWidgets 3.2.2+ library (https://www.wxwidgets.org/). Download the Windows ZIP version and extract somewhere convenient (e.g. the root of the C: drive). Note that the final extracted path cannot contain spaces.

Navigate to the extracted WxWidgets sources and go to the build\msw directory. Open the wx_vc16.sln solution file. Go to the "Build" menu and select "Batch Build...". In the batch build window, click the "Select All" button and then click "Build" (note that this will take some time). Once all files have been built, exit out of Visual Studio.

Finally, we need to add an environment variable to tell Visual Studio where to find the WxWidgets files. Open the Start menu, right click on "Computer" and select "Properties". Click on "Advanced System Settings" and then "Environment Variables". Add a new System Environment Variable named WX_WIN, and set its value equal to the full path to the WxWidgets directory (e.g. "C:\libraries\WxWidgets-3.2.2"). Click OK and exit out of the system properties windows.

Make sure that Visual Studio has been restarted so that it picks up the new environment variable.

##### LibPng and ZLib

You will need to obtain the LibPng 1.6.37 library - this can be found at https://download.sourceforge.net/libpng/lpng1637.zip. You will also need the ZLib source, download this from https://www.zlib.net/zlib-1.2.11.tar.gz.

Extract both archives to a common directory (e.g. C:\libraries). Once extracted, you should have a "lpng1637" folder and a "zlib-1.2.11" folder. Rename the zlib folder to "zlib" (i.e. remove the version from the folder name).

Navigate to lpng1637/projects/vstudio and open the zlib.props file with a text editor. Change the ```<TreatWarningAsError>``` value from ```true``` to ```false```. Save the file.

Next, open the vstudio.sln file. You will be prompted to upgrade the solution file to the latest version - choose "Yes". Once converted, go to the "Build" menu and select "Batch Build...". In the batch build window, click the "Select All" button and then click "Build". Once all files have been built, exit out of Visual Studio.

Finally, we need to add an environment variable to tell Visual Studio where to find libpng. Open the Start menu, right click on "Computer" and select "Properties". Click on "Advanced System Settings" and then "Environment Variables". Add a new System Environment Variable named LIBPNG_PATH, and set its value equal to the full path to libpng (e.g. "C:\libraries\libpng1637"). Click OK and exit out of the system properties windows.

Make sure that Visual Studio has been restarted so that it picks up the new environment variable.

### Build

Open the VS2019 solution file in landstalker_gfx\VC2019. Make sure that the appropriate configuration has been set (e.g. "Debug x86") and hit Ctrl+Shift+B to build.

# Screenshot

![edit](landstalker_edit.png)
