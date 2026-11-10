# This CPack script is invoked to build the Inno Setup installer for Tenacity.
# It requires CPACK_EXTERNAL_ENABLE_STAGING to be set, and you must check for
# the Inno Setup compiler yourself via find_program() and pass it to
# CPACK_TENACITY_INNO_SETUP_COMPILER.
#
# Internal variables:
#   * BUILD_DIR - should be set to CMAKE_BINARY_DIR by the caller
#   * OUTPUT_DIR - directory, where installer will be built
#
# Require variables:
#   * CPACK_TENACITY_INNO_SETUP_COMPILER - The INNO_SETUP compiler executable
#   * CPACK_TENACITY_INNO_SETUP_BUILD_CONFIG - The current build config if
#     using a single-config generator. For multi-config generators, the script
#     sets this to CPACK_BUILD_CONFIG.
#
# Optional parameters:
#   * CPACK_TENACITY_INNO_SETUP_SIGN - Whether or not to sign the installer.
#     * CPACK_TENACITY_INNO_SETUP_CERTIFICATE - Path to PFX file. If not
#       present, env:WINDOWS_CERTIFICATE will be used.
#     * CPACK_TENACITY_INNO_SETUP_CERTIFICATE_PASSWORD - Password for the PFX
#       file. If not present, env:WINDOWS_CERTIFICATE_PASSWORD will be used.
#   * CPACK_TENACITY_INNO_SETUP_EMBED_MANUAL - Whether or not to embed a fresh
#     copy of the manual.

# Allow if statements to use the new IN_LIST operator (compatibility override for CMake <3.3)
cmake_policy( SET CMP0057 NEW )

if (NOT CPACK_EXTERNAL_ENABLE_STAGING)
    message(FATAL_ERROR "CPack external staging is not enabled. This is a build bug")
endif()

if (NOT CPACK_TENACITY_INNO_SETUP_BUILD_CONFIG)
    set(CPACK_TENACITY_INNO_SETUP_BUILD_CONFIG ${CPACK_BUILD_CONFIG})
endif()

set(OUTPUT_DIR "${CPACK_TEMPORARY_DIRECTORY}")

# The .iss works out the version and sign config from the exe and env on its
# own, we hand it the arch. Point it at the config subdir, and keep the
# filename below in sync with OutputBaseFilename in the .iss.
set(TENACITY_BUILD_DIR "${OUTPUT_DIR}/${CPACK_TENACITY_INNO_SETUP_BUILD_CONFIG}")

# Don't trust CMAKE_SYSTEM_PROCESSOR for the arch. In here it's whatever host
# CPack is running on, so the x86 build on an x64 runner says AMD64 and we'd
# ship it as an x64 installer. Read the Machine field out of the exe instead.
set(_TENACITY_EXE "${TENACITY_BUILD_DIR}/Tenacity.exe")
if(NOT EXISTS "${_TENACITY_EXE}")
    message(FATAL_ERROR "Cannot find ${_TENACITY_EXE} to detect the installer architecture")
endif()
# e_lfanew at 0x3C is where the PE header starts, Machine is the 2 bytes
# right after the PE\0\0 signature. Both are little endian.
file(READ "${_TENACITY_EXE}" _pe_offset_hex OFFSET 60 LIMIT 4 HEX)
string(REGEX REPLACE "^(..)(..)(..)(..)$" "\\4\\3\\2\\1" _pe_offset_hex "${_pe_offset_hex}")
math(EXPR _pe_machine_offset "0x${_pe_offset_hex} + 4")
file(READ "${_TENACITY_EXE}" _pe_machine_hex OFFSET ${_pe_machine_offset} LIMIT 2 HEX)
if(_pe_machine_hex STREQUAL "6486")
    set(_INSTALLER_ARCH "x64")
elseif(_pe_machine_hex STREQUAL "64aa")
    set(_INSTALLER_ARCH "arm64")
elseif(_pe_machine_hex STREQUAL "4c01")
    set(_INSTALLER_ARCH "x86")
else()
    message(FATAL_ERROR "Unsupported PE machine type 0x${_pe_machine_hex} in ${_TENACITY_EXE}")
endif()
# .iss uses the exe's PE FILEVERSION (X.Y.Z); strip any pre-release suffix here.
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" _INSTALLER_VERSION "${CPACK_PACKAGE_VERSION}")
set(CPACK_EXTERNAL_BUILT_PACKAGES
    "${OUTPUT_DIR}/Output/tenacity-win-${_INSTALLER_VERSION}-${_INSTALLER_ARCH}.exe")

# Signing: opt-in via env vars read by the .iss.
if( CPACK_TENACITY_INNO_SETUP_SIGN )
    if( CPACK_TENACITY_INNO_SETUP_CERTIFICATE )
        set( ENV{WINDOWS_CERTIFICATE} "${CPACK_TENACITY_INNO_SETUP_CERTIFICATE}")
    endif()

    if( CPACK_TENACITY_INNO_SETUP_CERTIFICATE_PASSWORD )
        message("Setting env:WINDOWS_CERTIFICATE_PASSWORD...")
        set( ENV{WINDOWS_CERTIFICATE_PASSWORD}
             "${CPACK_TENACITY_INNO_SETUP_CERTIFICATE_PASSWORD}")
    endif()

    set( ENV{TENACITY_SIGN_SCRIPT}
         "${CPACK_TENACITY_SOURCE_DIR}/scripts/build/windows/PfxSign.ps1")
endif()

# Stage the .iss and auxiliary files next to the build tree
file(COPY "${CPACK_TENACITY_SOURCE_DIR}/win/Inno_Setup_Wizard/" DESTINATION "${OUTPUT_DIR}")

file(COPY "${CPACK_TENACITY_SOURCE_DIR}/resources" DESTINATION "${OUTPUT_DIR}/Additional")

file(COPY
        "${CPACK_TENACITY_SOURCE_DIR}/LICENSE.txt"
        "${CPACK_TENACITY_SOURCE_DIR}/win/tenacity.ico"
    DESTINATION
        "${OUTPUT_DIR}/Additional"
)

execute_process(
    COMMAND
        ${CPACK_TENACITY_INNO_SETUP_COMPILER}
            /Sbyparam=$p
            "/DBuildDir=${TENACITY_BUILD_DIR}"
            "/DTargetArch=${_INSTALLER_ARCH}"
            "tenacity.iss" /Qp
    WORKING_DIRECTORY
        ${OUTPUT_DIR}
    # When we upgrade to CMake min version 3.19 we can use this
    # COMMAND_ERROR_IS_FATAL ANY
)
