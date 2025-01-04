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

# Let's make this an object library so we don't deal with static library
# leftovers.
add_library(pffft OBJECT ${SOURCES})

# Yes, this is a hacky CMakeLists.txt
target_include_Directories(pffft PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Set this or the build could fail on some systems otherwise.
set_target_properties(pffft PROPERTIES POSITION_INDEPENDENT_CODE ON)
