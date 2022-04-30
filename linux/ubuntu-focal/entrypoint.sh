#!/usr/bin/env bash

conan --version

if [ ! -d "saucedacity" ]
then
    git clone https://github.com/saucedacity/saucedacity
fi
mkdir -p build

cd build

cmake_options=(
    -G "Unix Makefiles"
    -DCMAKE_BUILD_TYPE=Release
    -Dsaucedacity_lib_preference=system # Change the libs default to 'system'
    -Dsaucedacity_obey_system_dependencies=On # And force it!
    -Dsaucedacity_use_wxwidgets=local # wxWidgets 3.1 is not available
    -Dsaucedacity_use_expat=system
    -Dsaucedacity_use_lame=system
    -Dsaucedacity_use_sndfile=system
    -Dsaucedacity_use_soxr=system
    -Dsaucedacity_use_portaudio=local # System portaudio is not yet usable
    -Dsaucedacity_use_sqlite=local # We prefer using the latest version of SQLite
    -Dsaucedacity_use_ffmpeg=loaded
    -Dsaucedacity_use_id3tag=system # This library has bugs, that are fixed in *local* version
    -Dsaucedacity_use_mad=system # This library has bugs, that are fixed in *local* version
    -Dsaucedacity_use_nyquist=local # This library is not available
    -Dsaucedacity_use_vamp=local # The dev package for this library is not available
    -Dsaucedacity_use_ogg=system 
    -Dsaucedacity_use_vorbis=system
    -Dsaucedacity_use_flac=system
    -Dsaucedacity_use_lv2=system
    -Dsaucedacity_use_midi=system
    -Dsaucedacity_use_portmixer=local # This library is not available
    -Dsaucedacity_use_portsmf=system
    -Dsaucedacity_use_sbsms=local # We prefer using the latest version of sbsms
    -Dsaucedacity_use_soundtouch=system
    -Dsaucedacity_use_twolame=system
)

cmake "${cmake_options[@]}" ../saucedacity

exit_status=$?

if [ $exit_status -ne 0 ]; then
    exit $exit_status
fi

make -j`nproc`

cd bin/Release
mkdir -p "Portable Settings"

ls -la .
