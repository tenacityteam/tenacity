# Tenacity Build Instructions

## Prerequisites

### Linux

Most distributions do not package all of Tenacity's dependencies (yet).
wxWidgets 3.1 is required for building Tenacity but many distributions only
package wxWidgets 3.0. [PortMidi](https://github.com/portmidi/portmidi) and
[PortSMF](https://codeberg.org/tenacityteam/portsmf) are required for MIDI support
but some distributions do not package PortSMF (Tenacity can still build without
MIDI support). [libsbsms](https://github.com/claytonotey/libsbsms) is an
optional dependency used for time stretching that is not available in many Linux
distribution package managers either. Optionally,
[vcpkg can be used](#vcpkg-on-Linux) to build dependencies from source which
may be helpful if your distribution is missing some packages.

Installing ccache and ninja-build is highly recommended for faster builds but
not required. CMake will automatically use ccache if it is installed.

#### Debian, Ubuntu, and derived distributions

To install Tenacity's dependencies, run:

```
sudo apt-get install build-essential libavcodec-dev libavformat-dev libavutil-dev libflac++-dev libglib2.0-dev libgtk-3-dev libid3tag0-dev libjack-dev liblilv-dev libmad0-dev libmp3lame-dev libogg-dev libpng-dev portaudio19-dev libportmidi-dev libportsmf-dev libserd-dev libsndfile1-dev libsord-dev libsoundtouch-dev libsoxr-dev libsuil-dev libtwolame-dev vamp-plugin-sdk libvorbis-dev lv2-dev zlib1g-dev cmake ninja-build libjpeg-dev libtiff-dev liblzma-dev libsqlite3-dev
```

wxWidgets 3.1 is required but not packaged in Debian or Ubuntu. Refer to the
[wxWidgets documentation](https://docs.wxwidgets.org/latest/plat_gtk_install.html)
for how to install it from source code. 

> **NOTE**  The wxWidgets for GTK installation instructions don't mention that 
> you have to have all of the git sub-modules in place and at the right versions 
> before starting.  So make sure you clone it like this first:
> 
> ```
> $ git clone  --recurse-submodules  https://github.com/wxWidgets/wxWidgets.git
> $ cd wxWidgets
> ```

The above package list
includes wxWidgets' build dependencies. If you install wxWidgets
somewhere other than the default /usr/local, you need to set the
`WX_CONFIG` environment variable to the location of the `wx-config`
script installed by wxWidgets to get CMake to find wxWidgets 3.1. For
example, if you installed wxWidgets to /home/user/local:

```
export WX_CONFIG=/home/user/local/bin/wx-config
```

#### Fedora

To install Tenacity's dependencies, run:

```
sudo dnf install alsa-lib-devel cmake expat-devel flac-devel gcc-g++ gettext-devel lame-devel libid3tag-devel libmad-devel libogg-devel libsndfile-devel libvorbis-devel lilv-devel lv2-devel portaudio-devel portmidi-devel serd-devel sord-devel soundtouch-devel soxr-devel sqlite-devel sratom-devel suil-devel taglib-devel twolame-devel vamp-plugin-sdk-devel wxGTK-devel zlib-devel ccache ninja-build git ffmpeg-free-devel
```

If you use a high DPI screen, the wxWidgets 3.1.4 package in Fedora does
not work well for that. You can compile wxWidgets 3.1.5 instead of using
the Fedora package. Refer to the
[wxWidgets documentation](https://docs.wxwidgets.org/3.1/overview_cmake.html)
for details. If you install wxWidgets somewhere other than the default
/usr/local, you need to set the `WX_CONFIG` environment variable to
the location of the `wx-config` script installed by wxWidgets to get
CMake to find wxWidgets 3.1. For example, if you installed wxWidgets to
/home/user/local:

```
export WX_CONFIG=/home/user/local/bin/wx-config
```

#### Arch

Install `wxgtk3-dev-light` with your AUR helper of choice, for example:

```
paru -S wxgtk3-dev-light
```

CMake requires explicitly specifying the path to the wx-config script from
this AUR package:

```
export WX_CONFIG=/usr/bin/wx-config-gtk3-3.1
```

Install the rest of the build dependencies from the main Arch repository:

```
sudo pacman -S cmake ninja ccache expat gcc-libs gdk-pixbuf2 glibc flac gtk3 glib2 libid3tag lilv libmad libogg portaudio portmidi libsndfile libsoxr suil twolame vamp-plugin-sdk libvorbis soundtouch ffmpeg
```

TODO: add portsmf and sbsms to this package list when those packages are updated.

#### Alpine

The build dependencies for Tenacity and wxWidgets can be found on Alpine's
community repository:

```
sudo apk add cmake samurai lame-dev libsndfile-dev soxr-dev sqlite-dev portaudio-dev portmidi-dev libid3tag-dev soundtouch-dev libmad-dev ffmpeg-dev
```

wxWidgets 3.1 is required but not packaged in Alpine Linux. Refer to the
[wxWidgets documentation](https://github.com/wxWidgets/wxWidgets/blob/master/docs/gtk/install.md)
for how to install it from source code, and make sure to set
`--disable-xlocale` in the configuration.

To install wxWidgets' dependencies:

```
sudo apk add gtk+3.0-dev zlib-dev libpng-dev tiff-dev libjpeg-turbo-dev expat-dev
```

TODO: add portsmf and libsbsms to this package list when aports are accepted.


#### FreeBSD

wxWidgets 3.1.5 is packaged in FreeBSD's repositories. Install it and the rest
of Tenacity's dependencies:

```
sudo pkg install wx32-gtk3 cmake ninja pkgconf lame libsndfile libsoxr portaudio lv2 lilv suil vamp-plugin-sdk portmidi libid3tag twolame libmad soundtouch ffmpeg
```

#### vcpkg on Linux

Optionally, you can build dependencies from source using vcpkg, although vcpkg
is not set up to build GTK or GLib either for wxWidgets. To use vcpkg for
dependencies, pass `-D VCPKG=ON` to the CMake configure command. You will need
[nasm](https://www.nasm.us/) installed to build ffmpeg from vcpkg which you can
install from your distribution's package manager. If you use vcpkg, you need to
set the `WX_CONFIG` environment variable to the path of the wx-config script
installed by wxWidgets. For example, if you installed wxWidgets to /usr/local:

```
export WX_CONFIG=/usr/local/bin/wx-config
```

If you switch between system packages and vcpkg, you may need to delete
`CMakeCache.txt` inside your CMake build directory.

### Windows

Install
[Microsoft Visual Studio](https://visualstudio.microsoft.com/vs/community/)
with the **Desktop development with C++** installation option. **Be sure to
check the C++ MFC options to ensure the build succeeds too**.

Installing [sccache](https://github.com/mozilla/sccache) is highly recommended
for faster builds but not required. CMake will automatically use sccache if you
have it installed. You can install it from
[Chocolatey](https://community.chocolatey.org/packages/sccache):

```
choco install sccache
```

Alternatively, if you have a [Rust toolchain
installed](https://forge.rust-lang.org/infra/other-installation-methods.html),
you can build sccache from source:

```
cargo install sccache
```

Tenacity's dependencies will be built automatically with vcpkg when configuring
CMake. You can turn off vcpkg by passing `-D VCPKG=OFF` to the CMake
configuration command, but then it is up to you to install all of Tenacity's
dependencies.

Note that building the dependencies requires 10 GB of storage space.

### macOS

Install the Clang C++ compiler and macOS SDK from the Xcode command line tools.
If you have the Xcode IDE installed, you already have the command line tools
too. To install the Xcode command line tools without the Xcode IDE, launch the
Terminal application, and type the following command:

```
xcode-select --install
```

Click "Install" on the software update popup window that will appear and wait
for the download and installation to finish.

You will also need to install a few build tools and dependencies, which can be
installed from [Homebrew](https://brew.sh/):

```
brew install cmake ccache ninja nasm wxwidgets
```

The rest of the dependencies will be built automatically with vcpkg when
configuring CMake. You turn off vcpkg by passing `-D VCPKG=OFF` to the CMake
configuration command, but then it is up to you to install all of Tenacity's
dependencies.

## Building Tenacity

On Windows, run the commands below from the x64 Native Tools Command Prompt. For
other operating systems, run them from a normal shell.

First, download the Tenacity source code:

```
git clone --recurse-submodules https://codeberg.org/tenacityteam/tenacity
cd tenacity
```

This will clone Tenacity and all of its submodules, libnyquist and vcpkg. If
you don't want vcpkg, you can remove `--recurse-submodules` and do
`git submodule update --init lib-src/libnyquist` afterwards.

Then, configure CMake. This will take a long time the first time on macOS and
Windows (or if you use `-D VCPKG=ON` on Linux) because vcpkg will compile
itself, then compile Tenacity's dependencies. `-G Ninja` is recommended for
faster builds but not required. Add `-D CMAKE_INSTALL_PREFIX=/some/path` to
change the installation path from the default /usr/local:

```
cmake -G Ninja -S . -B build
```

Build Tenacity:

```
cmake --build build
```

Run Tenacity:

```
build/bin/Debug/tenacity
```

Optionally, install Tenacity:

```
cmake --install build
```

## Build options

  * **VCPKG** (ON|OFF): whether to use dependencies from vcpkg. ON by default
    for Windows and macOS; OFF by default for Linux.
  * **VCPKG_ROOT** (file path): path to vcpkg Git repository, defaults to
    using the vcpkg submodule in the Tenacity repository
  * **SCCACHE** (ON|OFF): whether to use sccache for compiler caching to
    speed up rebuilds. ON by default if sccache is installed.
  * **CCACHE** (ON|OFF): whether to use ccache for compiler caching to speed
    up rebuilds. ON by default if ccache is installed. If sccache and ccache
    are both installed, sccache will be prefered.
  * **PCH** (ON|OFF): Enables the use of precompiled headers. ON by default if
    either ccache or sccache was not found or was disabled.
  * **PERFORM_CODESIGN** (ON|OFF): Performs codesigning during the install step.
    This only works on Windows and macOS and requires the appropriate certificates
    to be installed in order for signing to work. **Note that codesigning and
    notarization might be broken as they haven't been tested**.
  * **PERFORM_NOTARIZATION** (ON|OFF): [macOS Only] Performs notarizaiton during
    the install step. This only works on macOS and if PERFORM_NOTARIZATION has
    been enabled.
  * **MANUAL_PATH** (file path): Path to the manual to package alongside DMG
    and InnoSEtup targets. If no path is specified (default), no manual is
    packaged.

The following feature options are enabled by default if the required libraries
are found. You may explicitly disable them if you prefer or your distribution
has outdated libraries that do not build with Tenacity.

  * **MIDI** (ON|OFF): MIDI support. Requires PortMidi and PortSMF.
  * **ID3TAG** (ON|OFF): ID3 tag support for MP3 files. Requires libid3tag.
  * **MP3_DECODING** (ON|OFF): MP3 decoding support. Requires libmad.
  * **MP2** (ON|OFF): MP2 codec support. Requires Twolame library.
  * **OGG** (ON|OFF): Ogg container format support. Requires libogg.
  * **VORBIS** (ON|OFF): Vorbis codec support. Requires libvorbis.
  * **FLAC** (ON|OFF): FLAC codec support. Requires libflac and libflac++ C++
    bindings.
  * **SBSMS** (ON|OFF): SBSMS timestretching support. Requires libsbsms.
  * **SOUNDTOUCH** (ON|OFF): SoundTouch timestretching support. Requires
    SoundTouch library.
  * **FFMPEG** (ON|OFF): Support for a wide variety of codecs with FFmpeg.
    Requires FFmpeg libraries.
  * **AUDIO_UNITS** (ON|OFF): Apple Audio Units plugin support. _macOS only._
  * **LADSPA** (ON|OFF): LADSPA plugin hosting support.
  * **LV2** (ON|OFF): LV2 plugin hosting support. Requires LV2, lilv, and
  * **VAMP** (ON|OFF): VAMP plugin hosting support. Requires VAMP host SDK.
    suil libraries.
  * **VST2** (ON|OFF): VST2 plugin hosting support. No libraries are required.
    ON by default.

### vcpkg Options

These options apply to all platforms where vcpkg is available. They are
available if you need further control over how Tenacity uses vcpkg if its usage
is required.

#### Settings a Custom Triplet

You may override the default triplet used for building Tenacity. This may be
useful for testing different triplets for different platforms.

To override the default triplet, set the `TENACITY_TRIPLET_OVERRIDE` variable
to the triplet you want to use. For example, if you're on Linux and want to
use the `x64-linux-dynamic` triplet, you would export the following:

```
export TENACITY_TRIPLET_OVERRIDE=x64-linux-dynamic
```

Overriding triplets might be required in some cases. For example, using static
builds of wxWidgets on Linux is not supported, but vcpkg builds static
libraries by default. The `x64-linux-dynamic` triplet must be used instead.
