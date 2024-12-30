# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileName: CMakeLists.txt
# SPDX-FileContributor: Dmitry Vedenko
#[[
A set of helpers for working with cryptographic functions.
]]

set( SOURCES
   crypto/SHA256.cpp
   crypto/SHA256.h
)
set( LIBRARIES
   PUBLIC
)
tenacity_library( lib-crypto "${SOURCES}" "${LIBRARIES}"
   "" "" )
