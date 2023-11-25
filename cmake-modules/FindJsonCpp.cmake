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
	pkg_check_modules(JsonCpp jsoncpp)
endif()

find_path(JsonCpp_INCLUDE_DIRS
    NAMES json
    PATHS ${JsonCpp_INCLUDE_DIRS}
    DOC "JsonCpp include directory"
)

find_library(JsonCpp_LIBRARIES
    NAMES jsoncpp
    PATHS ${JsonCpp_LIBRARIES} #LIBRARY_DIRS}
    DOC "JsonCpp library"
)

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

