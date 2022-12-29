#!/usr/bin/env bash

((${BASH_VERSION%%.*} >= 4)) || echo >&2 "$0: Warning: Using ancient Bash version ${BASH_VERSION}."

set -euxo pipefail

if [[ "${OSTYPE}" == darwin* ]]; then # macOS

    # Homebrew packages
    brew_packages=(
        bash # macOS ships with Bash v3 for licensing reasons so upgrade it now
        ccache
        ninja

        # needed to build ffmpeg
        nasm
    )
    brew install "${brew_packages[@]}"

else # Linux & others

    if ! which sudo; then
        function sudo() { "$@"; } # no-op sudo for use in Docker images
    fi

    # Distribution packages
    if which apt-get; then
        apt_packages=(
            # Build tools
            file
            g++
            ninja-build
            nasm
            git
            wget
            bash
            scdoc
            ccache

            # Dependencies
            debhelper-compat
            gettext
            libavcodec-dev
            libavformat-dev
            libavutil-dev
            libflac++-dev
            libglib2.0-dev
            libgtk-3-dev
            libid3tag0-dev
            libjack-dev
            liblilv-dev
            libmad0-dev
            libmp3lame-dev
            libogg-dev
            libpng-dev
            portaudio19-dev
            libportmidi-dev
            libportsmf-dev
            libserd-dev
            libsndfile1-dev
            libsord-dev
            libsoundtouch-dev
            libsoxr-dev
            libsuil-dev
            libtwolame-dev
            vamp-plugin-sdk
            libvorbis-dev
            lv2-dev
            zlib1g-dev
            libjpeg-dev
            libtiff-dev
            liblzma-dev
            libsqlite3-dev
        )

        sudo apt-get update -y
        sudo apt-get install -y --no-install-recommends "${apt_packages[@]}"
    else
        echo >&2 "$0: Error: You don't have a recognized package manager installed."
        exit 1
    fi
fi
