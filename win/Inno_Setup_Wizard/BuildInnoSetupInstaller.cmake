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

# Resolve any variables required by the setup script
if( CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|ARM64" )
    set( INSTALLER_SUFFIX "x64" )
    set( INSTALLER_X64_MODE "ArchitecturesInstallIn64BitMode=x64")
else()
    set( INSTALLER_SUFFIX "x86" )
    set( INSTALLER_X64_MODE "")
endif()

# Set the packages we're going to build
set(CPACK_EXTERNAL_BUILT_PACKAGES "${OUTPUT_DIR}/Output/${CPACK_PACKAGE_FILE_NAME}-${INSTALLER_SUFFIX}.exe")

if( CPACK_TENACITY_INNO_SETUP_SIGN )
    set( SIGN_TOOL "SignTool=byparam powershell -ExecutionPolicy Bypass -File \$q${CPACK_TENACITY_SOURCE_DIR}/scripts/build/windows/PfxSign.ps1\$q -File $f")

    if( CPACK_TENACITY_INNO_SETUP_CERTIFICATE )
        string(APPEND SIGN_TOOL " -CertFile \$q${CPACK_TENACITY_INNO_SETUP_CERTIFICATE}\$q")
    endif()

    if( CPACK_TENACITY_INNO_SETUP_CERTIFICATE_PASSWORD )
        message("Setting env:WINDOWS_CERTIFICATE_PASSWORD...")
        set( ENV{WINDOWS_CERTIFICATE_PASSWORD} "${CPACK_TENACITY_INNO_SETUP_CERTIFICATE_PASSWORD}")
    endif()
else()
    set( SIGN_TOOL )
endif()

if( EMBED_MANUAL )
    set ( MANUAL [[Source: "Package\help\manual\*"; DestDir: "{app}\help\manual\"; Flags: ignoreversion recursesubdirs]])
else()
    set( MANUAL )
endif()

# Prepare the output directory
# This is all set in the staging directory, but it's the same files from the
# normal build directory, so it shouldn't matter.
set(TENACITY_BUILD_DIR "${OUTPUT_DIR}")
set(TENACITY_EXE_LOCATION "${TENACITY_BUILD_DIR}/Tenacity.exe")

file(COPY "${CPACK_TENACITY_SOURCE_DIR}/win/Inno_Setup_Wizard/" DESTINATION "${OUTPUT_DIR}")
configure_file("${OUTPUT_DIR}/tenacity.iss.in" "${OUTPUT_DIR}/tenacity.iss")

# Copy additional files

file(COPY "${CPACK_TENACITY_SOURCE_DIR}/presets" DESTINATION "${OUTPUT_DIR}/Additional")

file(COPY
        "${CPACK_TENACITY_SOURCE_DIR}/LICENSE.txt"
        "${CPACK_TENACITY_SOURCE_DIR}/win/tenacity.ico"
    DESTINATION
        "${OUTPUT_DIR}/Additional"
)

execute_process(
    COMMAND
        ${CPACK_TENACITY_INNO_SETUP_COMPILER} /Sbyparam=$p "tenacity.iss" /Qp
    WORKING_DIRECTORY
        ${OUTPUT_DIR}
    # When we upgrade to CMake min version 3.19 we can use this
    # COMMAND_ERROR_IS_FATAL ANY
)
