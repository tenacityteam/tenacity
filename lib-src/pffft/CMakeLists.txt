# Add a CMakeLists.txt for PFFFT because...upstream's dependencies.
# Copyright (C) 2024 Tenacity Team and Contributors
# SPDX-License-Identifier: GPL-2.0-or-later

# Not really, but it's the same as Tenacity's, so lets make it consistent.
cmake_minimum_required(VERSION 3.16)

project(pffft LANGUAGES C)

set(SOURCES
    pffft.c
    pffft.h
    pfsimd_macros.h
)

# Let's make this a library - a static library
add_library(pffft STATIC ${SOURCES})

# Yes, this is a hacky CMakeLists.txt
target_include_Directories(pffft PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
