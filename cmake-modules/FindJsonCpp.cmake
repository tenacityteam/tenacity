# Copyright (C) 2023 Avery King
# Distributed under the GNU General Public License (GPL) version 2 or any later
# version. See the LICENSE.txt file for details

#[=======================================================================[.rst:

FindJsonCpp
-----------

Finds the jsoncpp library.

Imported Targets
^^^^^^^^^^^^^^^^

If found, this module provides the following imported targets:

``JsonCpp::JsonCpp``
The JsonCpp library.

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``JsonCpp_FOUND``
  True if JsonCpp was found on the system.

#]=======================================================================]

find_package(PkgConfig QUIET)
if (pkgConfig_FOUND)
    pkg_check_modules(JSONCPP_PC jsoncpp)
endif()

find_path(JsonCpp_INCLUDE_DIRS
    NAMES json/json.h
    PATHS ${JSONCPP_PC_INCLUDE_DIRS}
    PATH_SUFFIXES jsoncpp
    DOC "JsonCpp include directory"
)

mark_as_advanced(JsonCpp_INCLUDE_DIRS)

find_library(JsonCpp_LIBRARIES
    NAMES jsoncpp
    PATHS ${JSONCPP_PC_LIBRARIES}
    DOC "JsonCpp library"
)

mark_as_advanced(JsonCpp_INCLUDE_DIRS)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    JsonCpp
    DEFAULT_MSG
    JsonCpp_LIBRARIES
    JsonCpp_INCLUDE_DIRS
)

if (JsonCpp_FOUND)
    if (NOT TARGET JsonCpp::JsonCpp)
        add_library(JsonCpp::JsonCpp INTERFACE IMPORTED)
	target_link_libraries(JsonCpp::JsonCpp INTERFACE "${JsonCpp_LIBRARIES}")
	target_include_directories(JsonCpp::JsonCpp INTERFACE "${JsonCpp_INCLUDE_DIRS}")
    endif()
endif()

