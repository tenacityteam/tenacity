# Copyright (C) 2024 Avery King
# Distributed under the GNU General Public License (GPL) version 2 or any later
# version. See the LICENSE.txt file for details

#[=======================================================================[.rst:

FindRubberband
-----------

Finds the Rubberband timestretching library.

Imported Targets
^^^^^^^^^^^^^^^^

If found, this module provides the following imported targets:

``Rubberband::Rubberband``
The Rubberband library.

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``Rubberband_FOUND``
  True if Rubberband was found on the system.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig)
if (pkgConfig_FOUND)
    pkg_check_modules(RUBBERBAND_PC rubberband)
endif()

# Rubberband include dirs
find_path(
    Rubberband_INCLUDE_DIRS
    NAMES RubberBandStretcher.h
    PATHS ${RUBBERBAND_PC_INCLUDE_DIRS}
    PATH_SUFFIXES rubberband
)

# Rubberband libraries
find_library(
    Rubberband_LIBRARIES
    NAMES rubberband
)

find_package_handle_standard_args(
    Rubberband
    DEFAULT_MSG
    Rubberband_LIBRARIES
    Rubberband_INCLUDE_DIRS
)

if (Rubberband_FOUND)
    if (NOT TARGET Rubberband::Rubberband)
        add_library(Rubberband::Rubberband INTERFACE IMPORTED)
        target_link_libraries(Rubberband::Rubberband INTERFACE "${Rubberband_LIBRARIES}")
        target_include_directories(Rubberband::Rubberband INTERFACE "${Rubberband_INCLUDE_DIRS}")
    endif()
endif()
