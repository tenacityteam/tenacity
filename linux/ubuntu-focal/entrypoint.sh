#!/usr/bin/env bash

conan --version

if [ ! -d "tenacity" ]
then
    git clone https://codeberg.org/tenacityteam/tenacity
fi
mkdir -p build

cd build

cmake_options=(
    -G "Ninja"
    -DCMAKE_BUILD_TYPE=Release
)

cmake "${cmake_options[@]}" ../tenacity

exit_status=$?

if [ $exit_status -ne 0 ]; then
    exit $exit_status
fi

ninja -j`nproc`

cd bin/Release
mkdir -p "Portable Settings"

ls -la .
