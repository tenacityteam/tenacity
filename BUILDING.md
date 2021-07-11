# Saucedacity Build Instructions

## Prerequisites

* **python3** >= 3.5
* **conan** >= 1.32.0
* **cmake** >= 3.16
* A working C++ 14 compiler

### Conan

[The best way to install Conan is `pip`.](https://docs.conan.io/en/latest/installation.html)

To install Conan on Windows:

```
> pip install conan
```

To install Conan on macOS and Linux:

```
$ sudo pip3 install conan
```

Alternatively, on macOS, Conan is available from `brew`.

### CMake

#### Windows
On Windows, please use the [prebuilt binaries](https://cmake.org/download/).

#### macOS
On macOS, the easiest way to install CMake is through Homebrew. Run `brew install cmake` to install CMake.

#### Linux
On Linux, `cmake` is usually available from the system package manager. If you are building on an older LTS distro, e.g. Ubuntu 18.04, you will need to build and install your own version of CMake. On a more recent (LTS) distro, you shouldn't need to do this.

### Windows

We build Saucedacity using [Microsoft Visual Studio](https://visualstudio.microsoft.com/vs/community/). In order to build Saucedacity, the **Desktop development with C++** workload is required.

You will need at least MSVC 2017 to build Saucedacity on Windows using MSVC. We require a compiler that supports C++17.

#### Cygwin

Currently, building under Cygwin isn't supported. You can find instructions on the Audacity Wiki, but they are very old and likely won't work. If you somehow **do** get a working Saucedacity build, please let us know through our issue tracker.

#### MinGW

Building with MinGW isn't supported either. However, this might be of interest in the future as an alternative to building with Visual Studio and MSVC. Feel free to work on this.

### macOS

**WARNING**: these instructions might not work or might be incorrect.

You should be able to build Saucedacity using XCode 12. Apple Silicon is not supported as of now, but might be buildable

### Linux

We use GCC 9, but any C++17 compliant compiler should work.

On Debian or Ubuntu, you can install everything required using the following commands:

```
$ sudo apt-get update
$ sudo apt-get install -y build-essential cmake git python3-pip
$ sudo pip3 install conan
$ sudo apt-get install libgtk2.0-dev libasound2-dev libavformat-dev libjack-jackd2-dev uuid-dev
```

# Building Saucedacity

## Building on Windows

1. Clone Saucedacity from the Saucedacity's GitHub. 
  
   For example, in the **git-bash** run:

    ```
    $ git clone --recurse-submodules https://github.com/saucedacity/saucedacity/
    ```

2. Open CMake GUI. 
   
   Set the **Where is the source code** to the location where Saucedacity was cloned. 
   
   Set **Where to build the binaries** to the location you want to place your build in. It is preferred that this location is not within the directory with the source code.

3. Press **Configure**. You can choose which version of Visual Studio to use and the platform to build for in the pop-up. We support **x64** and **Win32** platforms. The **x64** platform is a default option. Press **Finish** to start the configuration process.

4. After successful configuration, you will see `Configuring done` in the last line of the log. Press **Generate** to generate the Visual Studio project. 

5. After you see "Generating done", press **Open Project** to open the project in Visual Studio.
   
6. Select "Build -> Build Solution".
   
7. You have now built Saucedacity...except that it might not work. See the additional steps below.

Generally, steps 1-5 are only needed the first-time you configure. Then, after you've generated the solution, you can open it in Visual Studio next time. If the project configuration has changed, the IDE will invoke CMake internally.

> Conan Center provides prebuilt binaries only for **x64**. Configuring the project for Win32 will take much longer, as all the 3rd party libraries will be built during the configuration.

## macOS

**Warning**: Saucedacity on macOS isn't tested. These build instructions might not work or Saucedacity might not build in the first place.

1. Clone Saucedacity from the Saucedacity GitHub project. 
  
    ```
    $ git clone --recurse-submodules https://github.com/saucedacity/saucedacity/
    ```

2. Configure Saucedacity using CMake:
   ```
   $ mkdir build && cd build
   $ cmake -G Xcode -T buildsystem=1 ../saucedacity
   ```

3. Open Saucedacity XCode project:
   ```
   $ open Audacity.xcodeproj
   ```
   and build Saucedacity using the IDE. 

Steps 1 and 2 are only required for first-time builds. 

Alternatively, you can use **CLion**. If you chose to do so, open the directory where you have cloned Saucedacity using CLion and you are good to go.

At the moment we only support **x86_64** builds. It is possible to build using AppleSilicon hardware but **mad** and **id3tag** should be disabled:

```
cmake -G Xcode -T buildsystem=1 -Duse_mad="off" -Duse_id3tag=off ../tenacity
```

## Linux & Other OS

1. Clone Saucedacity from the Saucedacity GitHub project. 
  
    ```
    $ git clone --recurse-submodules https://github.com/saucedacity/saucedacity/
    ```

2. Configure Saucedacity using CMake:
   ```
   $ mkdir build && cd build
   $ cmake -G "Unix Makefiles" -Duse_ffmpeg=loaded ../tenacity
   ```
   By default, Debug build will be configured. To change that, pass `-DCMAKE_BUILD_TYPE=Release` to CMake.

3. Build Saucedacity:
   ```
   $ make -j`nproc`
   ```

4. Testing the build:
   Adding a "Portable Settings" folder allows Saucedacity to ignore the settings of any existing Saucedacity (or Audacity) installation.
   ```
   $ cd bin/Debug
   $ mkdir "Portable Settings"
   $ ./saucedacity
   ```

5. Installing Saucedacity
   ```
   $ cd <build directory>
   $ sudo make install
   ```

# Advanced Build Options and Build Methods

## CMake options

You can use `cmake -LH` to get a list of the options available (or use CMake GUI or `ccmake`). The list will include documentation about each option. For convenience, [here is a list](CMAKE_OPTIONS.md) of the most notable options.

## Building using system libraries

On Linux it is possible to build Saucedacity using (almost) only the libraries provided by the package manager. Please, see the list of required libraries [here](linux/required_libraries.md).

```
$ mkdir build && cd build
$ cmake -G "Unix Makefiles" \
        -Duse_ffmpeg=loaded \
        -Dlib_preference=system \
        -Dobey_system_dependencies=On \
         ../tenacity
```

There are a few cases when the local library build is preferred or required:

1. **wxWidgets**: While Saucedacity on **Linux** uses vanilla version of wxWidgets, we **require** that version **3.1.3** is used. This version is not available in most of the distributions. We also support wxWidgets 3.2.0, however, but not all distributions support that yet either.
2. **portaudio-v19**: Saucedacity currently uses [some private APIs](https://github.com/audacity/audacity/issues/871), so using system portaudio is not yet possible.
3. **vamp-host-sdk**: Development packages are not available in Ubuntu 20.04.
4. **libnyquist** & **portmixer**: Libraries are not available in Ubuntu 20.04.
5. **sqlite3** & **libsmbs**: Libraries are very outdated in Ubuntu 20.04.

It is not advised to mix system and local libraries, except for the list above. `ZLib` is a very common dependency; it is possible to mix system and local libraries in one build. However, we advise against doing so.

### Using Docker

There is a [`Dockerfile`](linux/build-environment/Dockerfile) that can be used as an example of how to build Saucedacity using system libraries:

```
# NOTE: Docker builds are untested.
$ docker build -t audacity_linux_env .\linux\build-environment\
$ docker run --rm -v ${pwd}:/audacity/audacity/ -v ${pwd}/../build/linux-system:/audacity/build -it audacity_linux_env
```

To find system packages, we rely on `pkg-config`. There are several packages that have broken `*.pc` or do not use `pkg-config` at all. For the docker image - we handle this issue by installing the correct [`pc` files](linux/build-environment/pkgconfig/).

## Disabling Conan

Conan can be disabled completely using `-Dsaucedacity_conan_enabled=Off` during the configuration. 
This option implies `-Dsaucedacity_obey_system_dependencies=On` and disables `local` for packages that are managed with Conan.

Note that Conan is intended to be removed in the future. In turn, Saucedacity will be built using mostly system libraries and some local libraries (most notably libmad).

## Other Architectures and Cross Compiling
Saucedacity does not support cross-compiling officially, but it has some interest. Keep in mind that you are on your own here and nothing is expected to work. If cross-compiling Saucedacity somehow does work and you would like to distribute those binaries, we make a (non-binding) request that you indicate that they are unofficial binaries (e.g., you could say *[insert some username here]'s Saucedacity builds* and that will fulfill our (once again non-binding) request). The only requirement is that you follow Saucedacity's license, the GPLv2 or later. That is in case you had to modify any part of Saucedacity to work.

We are interested in looking at different architectures, most notably ARM. For macOS, this will be a requirement in the future. For Windows, Linux, and other OSes, this will be of interest and maybe a hobby. Of course, we do not supported these other architectures *except* for ARM (even though we don't publically distribute ARM builds). ARM builds will receieve the same support for any other officially supported architecture, so long as you haven't made any modifications.
