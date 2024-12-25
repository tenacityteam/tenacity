# Minimum required is 3.16 due to use of multiple values in
# generator expressions and the use of precompiled headers
cmake_minimum_required( VERSION 3.16 )

find_package(Git)

if(WIN32 OR APPLE)
  option(VCPKG "Use vcpkg for dependencies" ON)
else()
  option(VCPKG "Use vcpkg for dependencies" OFF)
endif()

if(VCPKG)
  set(ENV{VCPKG_DISABLE_METRICS} true)

  if(VCPKG_ROOT)
    message(STATUS "Using dependencies from vcpkg repository at ${VCPKG_ROOT}")
    if(NOT EXISTS "${VCPKG_ROOT}/bootstrap-vcpkg.sh")
      message(FATAL_ERROR "${VCPKG_ROOT} is not a vcpkg Git repository.")
    endif()
  else()
    message(STATUS "Using dependencies from vcpkg Git submodule")
    set(VCPKG_ROOT "${CMAKE_SOURCE_DIR}/vcpkg")

    if(NOT EXISTS "${VCPKG_ROOT}/bootstrap-vcpkg.sh")
      message(STATUS "Initializing vcpkg Git submodule")
      execute_process(COMMAND ${GIT_EXECUTABLE} submodule init WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
      execute_process(COMMAND ${GIT_EXECUTABLE} submodule update WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    endif()
  endif()

  if(NOT DEFINED VCPKG_OVERLAY_PORTS)
    set(VCPKG_OVERLAY_PORTS "${VCPKG_ROOT}/overlay/ports")
  endif()

  if(NOT DEFINED VCPKG_OVERLAY_TRIPLETS)
    set(VCPKG_OVERLAY_TRIPLETS "${VCPKG_ROOT}/overlay/triplets")
  endif()

  if (DEFINED ENV{TENACITY_TRIPLET_OVERRIDE})
    set(VCPKG_TARGET_TRIPLET "$ENV{TENACITY_TRIPLET_OVERRIDE}")
    message(STATUS "vcpkg triplet overriden. Using triplet '$ENV{TENACITY_TRIPLET_OVERRIDE}'")
  else()
    if(NOT DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
      if(APPLE)
         if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64") # Apple Silicon
            set(VCPKG_TARGET_TRIPLET "arm64-osx-dynamic")
         else() # Intel
            set(VCPKG_TARGET_TRIPLET "x64-osx-dynamic")
         endif()
      elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
        set(VCPKG_TARGET_TRIPLET "x64-linux-dynamic")
      endif()
    elseif(DEFINED ENV{VCPKG_DEFAULT_TRIPLET})
        set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}")
    endif()
  endif()

  set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
else()
  message(STATUS "Searching for dependencies from system, not using vcpkg.")
endif()

# If building with GNU compiler, then must be 6 or later.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS "6")
   message(FATAL_ERROR "Tenacity requires at least GCC 6")
endif ()

if( EXISTS "${CMAKE_SOURCE_DIR}/cmake-proxies/cmake-modules/Variables.cmake" )
   include( "${CMAKE_SOURCE_DIR}/cmake-proxies/cmake-modules/Variables.cmake" )
endif()

# We only do alpha builds, beta builds, and release versions.
# Most of the time we're in development, so TENACITY_BUILD_LEVEL should be
# defined to 0.
# Its value may be more than 0 for pre-release "Beta" builds that differ only
# in the welcome screen, and hiding of some development menu commands, but
# still link to the alpha manual online.

# Set this value to 0 for alpha, 1 for beta, 2 for release builds
if( NOT DEFINED TENACITY_BUILD_LEVEL )
   set( TENACITY_BUILD_LEVEL 0 CACHE STRING "0 for alpha, 1 for beta, 2 for release builds" )
endif()

if( TENACITY_BUILD_LEVEL EQUAL 0 )
   set( TENACITY_SUFFIX "-alpha" )
elseif(  TENACITY_BUILD_LEVEL EQUAL 1 )
   set( TENACITY_SUFFIX "-beta" )
else()
   set( TENACITY_SUFFIX "" )
endif()

# Don't allow in-source builds...no real reason, just
# keeping those source trees nice and tidy.  :-)
# (This can be removed if it becomes an issue.)
if( "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}" )
   message( FATAL_ERROR
      "In-source builds not allowed.\n"
      "Create a new directory and run cmake from there, i.e.:\n"
      "  mkdir build\n"
      "  cd build\n"
      "  cmake ..\n"
      "You will need to delete CMakeCache.txt and CMakeFiles from this directory to clean up."

   )
endif()

# Default build type is 'Debug' if CMAKE_BUILD_TYPE and
# CMAKE_CONFIGURATION_TYPES are not set AND TENACITY_BUILD_LEVEL is less
# than 2 (meaing alpha or beta).
if( NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES )
   if ( TENACITY_BUILD_LEVEL LESS 2)
     set( CMAKE_BUILD_TYPE "Debug" )
   else()
     set( CMAKE_BUILD_TYPE "Release" )
   endif()
endif()

# Ignore COMPILE_DEFINITIONS_<Config> properties
cmake_policy( SET CMP0043 NEW )

# Link libraries by full path even in implicit directories.
cmake_policy( SET CMP0060 NEW )

# ``INTERPROCEDURAL_OPTIMIZATION`` is enforced when enabled.
cmake_policy( SET CMP0069 NEW )

# ``FindOpenGL`` prefers GLVND by default when available.
cmake_policy( SET CMP0072 NEW )

# Include file check macros honor ``CMAKE_REQUIRED_LIBRARIES``.
cmake_policy( SET CMP0075 NEW )

if(CMAKE_VERSION VERSION_GREATER "3.21")
  # ``cmake_dependent_option()`` supports full :ref:`Condition Syntax`.
  cmake_policy( SET CMP0127 NEW )
endif()

# Definitions that must happen before the project() command
if( APPLE )
   if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")
      set(MIN_MACOS_VERSION 11)
   else()
      set( MIN_MACOS_VERSION 10.15 )
   endif()
   set( TARGET_MACOS_VERSION 11 )
   # Generate schema files
   set( CMAKE_XCODE_GENERATE_SCHEME ON )

   # Define the OSX compatibility parameters
   set( CMAKE_OSX_DEPLOYMENT_TARGET ${MIN_MACOS_VERSION} CACHE INTERNAL "" )
   set( CMAKE_OSX_SYSROOT macosx CACHE INTERNAL "" )
   set( CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "" CACHE INTERNAL "" )

   # Shouldn't cmake do this???
   string( APPEND CMAKE_CXX_FLAGS " -stdlib=libc++" )
endif()

# Add our module path
set( TENACITY_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake-proxies/cmake-modules")
set( CMAKE_MODULE_PATH
   ${TENACITY_MODULE_PATH}
   ${CMAKE_MODULE_PATH}
   "${CMAKE_SOURCE_DIR}/cmake-modules"
)

set( CMAKE_PREFIX_PATH
   ${CMAKE_BINARY_DIR}
   ${CMAKE_PREFIX_PATH}
)

# This "is a good thing" but greatly increases link time on Linux
#set( CMAKE_INTERPROCEDURAL_OPTIMIZATION ON )
#set( CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON )
#set( CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF )

# Set the required C++ standard
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

include( CMakeDependentOption )

project( Tenacity )

# Use ccache if available
find_program( CCACHE_PROGRAM ccache )
mark_as_advanced( FORCE CCACHE_PROGRAM )

find_program( SCCACHE_PROGRAM sccache )
mark_as_advanced( FORCE SCCACHE_PROGRAM )

if( CCACHE_PROGRAM )
   message( STATUS "Found ccache: ${CCACHE_PROGRAM}" )
   set_property( GLOBAL PROPERTY CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
   set_property( GLOBAL PROPERTY CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" )
elseif( SCCACHE_PROGRAM )
   message( STATUS "Found sccache: ${SCCACHE_PROGRAM}" )
   set_property( GLOBAL PROPERTY CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
   set_property( GLOBAL PROPERTY CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}" )
else()
   message( STATUS "Could NOT find ccache nor sccache, no compiler caching enabled" )
endif()

if(NOT "${SCCACHE_PROGRAM}" STREQUAL "SCCACHE_PROGRAM-NOTFOUND")
  option(SCCACHE "Use sccache for compiler caching to speed up rebuilds." ON)
endif()

if(NOT "${CCACHE_PROGRAM}" STREQUAL "CCACHE_PROGRAM-NOTFOUND")
  option(CCACHE "Use ccache for compiler caching to speed up rebuilds." ON)
endif()

# Prefer sccache if both ccache and sccache are found because Windows users may have
# ccache installed with MinGW which would not work with MSVC.
if(SCCACHE)
   message( STATUS "Using sccache for compiler caching to speed up rebuilds" )
   set( CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}" )
   set( CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}" )
elseif(CCACHE)
   message( STATUS "Using ccache for compiler caching to speed up rebuilds" )
   set( CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" )
   set( CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" )
else()
   message( STATUS "No compiler caching enabled. Install sccache or ccache to enable compiler caching." )
endif()

# Load our functions/macros
include( TenacityFunctions )

set_from_env(TENACITY_ARCH_LABEL) # e.g. x86_64

# Pull all the modules we'll need
include( CheckCXXCompilerFlag )
include( CheckIncludeFile )
include( CheckIncludeFiles )
include( CheckLibraryExists )
include( CheckSymbolExists )
include( CheckTypeSize )
include( CMakeDependentOption )
include( CMakeDetermineASM_NASMCompiler )
include( CMakePushCheckState )
include( GNUInstallDirs )
include( TestBigEndian )

# Determine 32-bit or 64-bit target
if( CMAKE_C_COMPILER_ID MATCHES "MSVC" AND CMAKE_VS_PLATFORM_NAME MATCHES "Win64|x64" )
   set( IS_64BIT ON )
elseif( NOT CMAKE_SIZEOF_VOID_P STREQUAL "4" )
   set( IS_64BIT ON )
endif()

message( STATUS "Build Info:" )
message( STATUS "  Host System: ${CMAKE_HOST_SYSTEM}" )
message( STATUS "  Host System Name: ${CMAKE_HOST_SYSTEM_NAME}" )
message( STATUS "  Host System Processor: ${CMAKE_HOST_SYSTEM_PROCESSOR}" )
message( STATUS "  Host System Version: ${CMAKE_HOST_SYSTEM_VERSION}" )

if( IS_64BIT )
   message( STATUS "  Host System Architecture: 64-bit" )
else()
   message( STATUS "  Host System Architecture: 32-bit" )
endif()

message( STATUS )
message( STATUS "  Compiler: ${CMAKE_CXX_COMPILER}" )
message( STATUS "  Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}" )
message( STATUS "  Compiler Standard: ${CMAKE_CXX_STANDARD}" )
message( STATUS "  Compiler Standard Required: ${CMAKE_CXX_STANDARD_REQUIRED}" )
message( STATUS "  Compiler Extensions: ${CMAKE_CXX_EXTENSIONS}" )
message( STATUS )

if( CMAKE_GENERATOR MATCHES "Visual Studio" )
   message( STATUS "  MSVC Version: ${MSVC_VERSION}" )
   message( STATUS "  MSVC Toolset: ${MSVC_TOOLSET_VERSION}" )
   message( STATUS )
elseif( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
   if( CMAKE_GENERATOR MATCHES "Xcode" )
      message( STATUS "  Xcode Version: ${XCODE_VERSION}" )
   endif()
   message( STATUS "  MacOS SDK: ${CMAKE_OSX_SYSROOT}" )
   message( STATUS )

endif()

# Current version and commit info
set( TENACITY_VERSION 1 )
set( TENACITY_RELEASE 4 )
set( TENACITY_REVISION 0 )
set( GIT_DESCRIBE "unknown" )
find_package( Git QUIET )
if( GIT_FOUND )
    execute_process(
        COMMAND
            ${GIT_EXECUTABLE} describe --abbrev=7 --tags
        WORKING_DIRECTORY
            ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE
            GIT_DESCRIBE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Get the number of commits since the last tag and the current commit from
    # Git describe. This is only if we're not building a release version.
    if( GIT_DESCRIBE AND TENACITY_BUILD_LEVEL LESS 2)
        string( REGEX MATCHALL "-.*" GIT_VERSION_TAG "${GIT_DESCRIBE}" )
    endif()
endif()
message( STATUS "  Current Commit: ${GIT_DESCRIBE}" )
message( STATUS )

# Organize subdirectories/targets into folders for the IDEs
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

if( CMAKE_GENERATOR MATCHES "Visual Studio" )
   # Make sure Tenacity is the startup project
   set_directory_properties(
      PROPERTIES
         VS_STARTUP_PROJECT "${CMAKE_PROJECT_NAME}"
   )

   # Build using multiple processors
   foreach( config ${CMAKE_CONFIGURATION_TYPES} )
      string( TOUPPER "${config}" config )
      string( APPEND CMAKE_C_FLAGS_${config} " /MP" )
      string( APPEND CMAKE_CXX_FLAGS_${config} " /MP" )
   endforeach()

   # Define system library information, but we'll do the install
   set( CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP YES )
   set( CMAKE_INSTALL_UCRT_LIBRARIES NO )
   set( CMAKE_INSTALL_MFC_LIBRARIES NO )
   set( CMAKE_INSTALL_OPENMP_LIBRARIES NO )
endif()
if( CMAKE_SYSTEM_NAME MATCHES "Windows" )
   include( InstallRequiredSystemLibraries )
endif()

# Where the final product is stored
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

set ( _SHARED_PROXY_BASE "shared" )
set ( _SHARED_PROXY_BASE_PATH "${CMAKE_BINARY_DIR}/${_SHARED_PROXY_BASE}")

# Define the non-install and executable paths
if( CMAKE_CONFIGURATION_TYPES )
   set( _DESTDIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}" )
   set( _SHARED_PROXY_PATH "${_SHARED_PROXY_BASE_PATH}/${CMAKE_CFG_INTDIR}")
else()
   set( _DESTDIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}" )
   set( _SHARED_PROXY_PATH "${_SHARED_PROXY_BASE_PATH}/${CMAKE_BUILD_TYPE}")
endif()

set( _DEST "${_DESTDIR}" )
set( INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" )
set( _LIBDIR "${CMAKE_INSTALL_LIBDIR}" )
set( _DATADIR "${CMAKE_INSTALL_DATADIR}" )
set( _PKGLIB "${_LIBDIR}/tenacity" )
set( _PKGDATA "${_DATADIR}/tenacity/" )
set( _MANDIR "${CMAKE_INSTALL_MANDIR}" )
set( _MODDIR "${_DEST}/modules" )
set( _EXEDIR "${_DEST}" )

# Setup RPATH handling
set( CMAKE_BUILD_RPATH "${_DEST}/${_PKGLIB}" )
set( CMAKE_BUILD_WITH_INSTALL_RPATH FALSE )
set( CMAKE_INSTALL_RPATH "$ORIGIN/../${_PKGLIB}" )
set( CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE )

# Adjust them for the Mac
if( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
   set( _APPDIR "Tenacity.app/Contents" )
   set( _DEST "${_DESTDIR}/${_APPDIR}" )
   set( _EXEDIR "${_DEST}/MacOS" )
   set( _MODDIR "${_DEST}/modules" )
   set( _PKGLIB "${_DEST}/Frameworks" )

   set( CMAKE_MACOSX_RPATH OFF )
endif()

# Add the math library (if found) to the list of required libraries
check_library_exists( m pow "" HAVE_LIBM )
if( HAVE_LIBM )
   list( APPEND CMAKE_REQUIRED_LIBRARIES -lm )
endif()

check_library_exists( atomic __atomic_fetch_add_4 "" HAVE_LIBATOMIC )
if( HAVE_LIBATOMIC )
  list( APPEND CMAKE_REQUIRED_LIBRARIES -latomic )
endif()

# Add the dynamic linker library (if needed) to the list of required libraries
list( APPEND CMAKE_REQUIRED_LIBRARIES ${CMAKE_DL_LIBS} )

# Make sure they're used during the link steps
set( CMAKE_LINK_INTERFACE_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} )

# Various common checks whose results are used by the different targets
test_big_endian( WORDS_BIGENDIAN )

# Check for compiler flags
set( MMX_FLAG "" CACHE INTERNAL "" )
set( SSE_FLAG "" CACHE INTERNAL "" )
if( CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU" )
   check_cxx_compiler_flag( "-mmmx" HAVE_MMX )
   if( HAVE_MMX AND NOT IS_64BIT )
      set( MMX_FLAG "-mmmx" CACHE INTERNAL "" )
   endif()

   check_cxx_compiler_flag( "-msse" HAVE_SSE )
   if( HAVE_SSE AND NOT IS_64BIT )
      set( SSE_FLAG "-msse" CACHE INTERNAL "" )
   endif()

   check_cxx_compiler_flag( "-msse2" HAVE_SSE2 )
   if( HAVE_SSE2 AND NOT IS_64BIT )
      set( SSE_FLAG "-msse2" CACHE INTERNAL "" )
   endif()
elseif( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
   set( HAVE_MMX ON )
   set( HAVE_SSE ON )
   set( HAVE_SSE2 ON )
   if( NOT IS_64BIT )
      set( SSE_FLAG "/arch:SSE2" )
   endif()

   # required for sccache
   if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
      string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
      string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
      string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
      string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
      string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    endif()
endif()

check_include_file( "pa_jack.h" PA_USE_JACK)
check_include_file( "pa_win_wmme.h" PA_USE_WMME )

check_symbol_exists( lrint "math.h" HAVE_LRINT )
check_symbol_exists( lrintf "math.h" HAVE_LRINTF )
check_symbol_exists( mlock "sys/mman.h" HAVE_MLOCK )

# We'll be using it if it's available
find_package( PkgConfig QUIET )

# Mostly just to make the CMP0072 policy happy
find_package( OpenGL QUIET )

# Precreate the lib and lib64 directories so we can make then the same
if( NOT EXISTS "${CMAKE_BINARY_DIR}/lib" )
   file( MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/lib" )
endif()

# Only create on systems that need it, effectively excluding Windows where links
# may not work due to insufficient privileges
if( NOT CMAKE_INSTALL_LIBDIR STREQUAL "lib" AND NOT EXISTS "${CMAKE_BINARY_DIR}/lib64" )
   file( CREATE_LINK "${CMAKE_BINARY_DIR}/lib" "${CMAKE_BINARY_DIR}/lib64" SYMBOLIC )
endif()

# Define Tenacity's name
if( CMAKE_SYSTEM_NAME MATCHES "Darwin|Windows" )
   set( APP_NAME "Tenacity" )
else()
   set( APP_NAME "tenacity" )
   set( PROPER_NAME "Tenacity" )
endif()

# Define Application ID
set( APP_ID "org.tenacityaudio.Tenacity" )

# Create short and full version strings
set( TENACITY_DIST_VERSION ${TENACITY_VERSION}.${TENACITY_RELEASE}.${TENACITY_REVISION} )
set( TENACITY_INFO_VERSION ${TENACITY_VERSION}.${TENACITY_RELEASE}.${TENACITY_REVISION} )

# Build our custom libraries as either static or shared
# NOTE: this does not affect external libraries (e.g., wxWidgets).
option(BUILD_STATIC_LIBS "Build Tenacity's libraries as static libraries" OFF)

if (${BUILD_STATIC_LIBS})
   message(STATUS "Building static Tenacity libraries")
else()
   message(STATUS "Building shared Tenacity libraries")
endif()

# Python is possibly used for message catalogs
find_package( Python3 )
if( Python3_FOUND )
   set( PYTHON "${Python3_EXECUTABLE}" )
endif()

find_package(JsonCpp REQUIRED)
find_package(libzip REQUIRED)
find_package(ZLIB REQUIRED)
find_package(EXPAT REQUIRED)
find_package(mp3lame REQUIRED)
find_package(SndFile REQUIRED)
find_package(Soxr REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(PortAudio REQUIRED)

find_package(PortMidi)
find_package(PortSMF)
cmake_dependent_option(MIDI "MIDI support requires PortMidi and PortSMF." ON "PortMidi_FOUND;PortSMF_FOUND" OFF)
if(MIDI)
    set(USE_MIDI ON)
    message(STATUS "MIDI support enabled.")
else()
    message(STATUS "MIDI support disabled. Requires both PortMidi and PortSMF.")
endif()

find_package(id3tag)
cmake_dependent_option(ID3TAG "ID3 Tag support for MP3s." ON "id3tag_FOUND" OFF)
if(ID3TAG)
    set(USE_LIBID3TAG ON)
    message(STATUS "ID3 tag support for MP3s enabled.")
else()
    message(STATUS "ID3 tag support for MP3s disabled. Requires libid3tag.")
endif()

find_package(MAD)
cmake_dependent_option(MP3_DECODING "MP3 decoding support with libmad" ON "MAD_FOUND" OFF)
if(MP3_DECODING)
    set(USE_LIBMAD ON)
    message(STATUS "MP3 decoding support enabled.")
else()
    message(STATUS "MP3 decoding support disabled. Requires libmad.")
endif()

find_package(libtwolame)
cmake_dependent_option(MP2 "MP2 support with Twolame" ON "libtwolame_FOUND" OFF)
if(MP2)
    set(USE_LIBTWOLAME ON)
    message(STATUS "MP2 encoding support enabled.")
else()
    message(STATUS "MP2 encoding support disabled. Requires Twolame library.")
endif()

find_package(Matroska)
cmake_dependent_option(MATROSKA "MKA/MKV container format support" ON "Matroska_FOUND" OFF)
if(MATROSKA)
    set(USE_LIBMATROSKA ON)
    message(STATUS "MKA/MKV container format support enabled.")
else()
    message(STATUS "MKA/MKV container format support disabled. Requires libmatroska.")
endif()

find_package(Ogg)
cmake_dependent_option(OGG "OGG container format support" ON "Ogg_FOUND" OFF)
if(OGG)
    set(USE_LIBOGG ON)
    message(STATUS "OGG container format support enabled.")
else()
    message(STATUS "OGG container format support disabled. Requires libogg.")
endif()

find_package(Vorbis)
cmake_dependent_option(VORBIS "Vorbis codec support" ON "Vorbis_FOUND" OFF)
if(VORBIS)
    set(USE_LIBVORBIS ON)
    message(STATUS "Vorbis codec support enabled.")
else()
    message(STATUS "Voribs codec support disabled. Requires libvorbis.")
endif()

find_package(FLAC++)
cmake_dependent_option(FLAC "FLAC codec support" ON "FLAC++_FOUND" OFF)
if(FLAC)
    set(USE_LIBFLAC ON)
    message(STATUS "FLAC codec support enabled.")
else()
    message(STATUS "FLAC codec support disabled. Requires libflac and libflac++ C++ bindings.")
endif()

find_package(sbsms)
if (VCPKG)
    cmake_dependent_option(SBSMS "SBSMS timestretching" ON "sbsms_FOUND" OFF)
else()
    cmake_dependent_option(SBSMS "SBSMS timestretching" OFF "sbsms_FOUND" OFF)
endif()
if(SBSMS)
    set(USE_SBSMS ON)
    message(STATUS "SBSMS timestretching support enabled.")
else()
    message(STATUS "SBSMS timestretching support disabled. Requires libsbsms.")
endif()

find_package(SoundTouch)
cmake_dependent_option(SOUNDTOUCH "SoundTouch timestretching" ON "SoundTouch_FOUND" OFF)
if(SOUNDTOUCH)
    set(USE_SOUNDTOUCH ON)
    message(STATUS "SoundTouch timestretching support enabled.")
else()
    message(STATUS "SoundTouch timestretching support disabled. Requires SoundTouch library.")
endif()

option(FFMPEG "FFMPEG codecs support." ON)
if(FFMPEG)
    set(USE_FFMPEG ON)
    message(STATUS "FFMPEG codecs support enabled.")
else()
    message(STATUS "FFMPEG codecs support disabled.")
endif()

find_package(VampHostSDK)
cmake_dependent_option(VAMP "VAMP plugin hosting." ON "VampHostSDK_FOUND" OFF)
if(VAMP)
    set(USE_VAMP ON)
    message(STATUS "VAMP plugin hosting enabled.")
else()
    message(STATUS "VAMP plugin hosting disabled. Requires VAMP host SDK.")
endif()

find_package(LV2)
find_package(lilv)
find_package(suil)
cmake_dependent_option(LV2 "LV2 plugin host support" ON "LV2_FOUND;lilv_FOUND;suil_FOUND" OFF)
if(LV2)
    message(STATUS "LV2 plugin hosting enabled.")
    set(USE_LV2 ON)
else()
    message(STATUS "LV2 plugin hosting disabled. Requires LV2, lilv, and suil libraries.")
endif()

option(LADSPA "Enable LADSPA plugin support" ON)
if (LADSPA)
   set(USE_LADSPA ON)
   message(STATUS "LADSPA plugin hosting enabled.")
else()
   message(STATUS "LADSPA plugin hosting disabled.")
endif()

option(VST2 "VST2 plugin host support" ON)
if(VST2)
    set(USE_VST2 ON)
    message(STATUS "VST2 plugin host support enabled.")
else()
    set(USE_VST2 OFF)
    message(STATUS "VST2 plugin host support disabled.")
endif()

if(NOT CMAKE_SYSTEM_NAME MATCHES "Darwin|Windows")
    find_package(GLIB REQUIRED)
    find_package(GTK 3.0 REQUIRED)

endif()

# Handle Audio Units option
if( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
   option(AUDIO_UNITS "Enable Audio Units plugin support." ON)
   set(USE_AUDIO_UNITS ${use_audio_units})
   if (AUDIO_UNITS)
      message(STATUS "Audio Units support enabled.")
   else()
      message(STATUS "Audio Units support disabled.")
   endif()
endif()

# define EXPERIMENTAL flags
include("src/Experimental.cmake")

find_package(wxWidgets 3.1.5 REQUIRED COMPONENTS base core html net)
include(${wxWidgets_USE_FILE})
# The FindwxWidgets.cmake module does not create an IMPORT target, so hack one together.
# This makes it easy to add the compile definitions to the lib-strings and lib-strings-utils targets.
if(NOT TARGET wxWidgets::wxWidgets)
  add_library(wxWidgets::wxWidgets INTERFACE IMPORTED)
  target_link_libraries(wxWidgets::wxWidgets INTERFACE ${wxWidgets_LIBRARIES})
  target_compile_definitions(wxWidgets::wxWidgets INTERFACE ${wxWidgets_DEFINITIONS} ${wxWidgets_DEFINITIONS_DEBUG})
endif()

option(NYQUIST "Enable Nyquist support" ON)
if (NYQUIST)
   set(USE_NYQUIST ON)
   add_subdirectory(lib-src/libnyquist)
   add_subdirectory("nyquist")
   add_subdirectory("plug-ins")

   if (NOT EXISTS "${CMAKE_SOURCE_DIR}/lib-src/libnyquist/CMakeLists.txt")
      message(FATAL_ERROR "libnyquist submodule not initialized. Please initialize it.")
   else()
      message(STATUS "Nyquist support enabled")
   endif()
else()
   set(USE_NYQUIST OFF)
   message(STATUS "Nyquist support disabled")
endif()

# Manual Packaging

set(MANUAL_PATH "" CACHE STRING "Path to the manual to package DMG and InnoSetup targets with")
if (NOT "${MANUAL_PATH}" STREQUAL "")
   set(PACKAGE_MANUAL ON)
else()
   set(PACKAGE_MANUAL OFF)
endif()

add_subdirectory( "help" )
add_subdirectory( "images" )
add_subdirectory( "libraries" )
add_subdirectory( "locale" )
add_subdirectory( "src" )
add_subdirectory( "modules" )
# add_subdirectory( "nyquist" )
# add_subdirectory( "plug-ins" )
add_subdirectory( "scripts" )

# Generate config file
if( CMAKE_SYSTEM_NAME MATCHES "Windows" )
   configure_file( src/tenacity_config.h.in src/private/configwin.h )
elseif( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
   set( HAVE_VISIBILITY 1 )
   configure_file( src/tenacity_config.h.in src/private/configmac.h )
else()
   set( HAVE_VISIBILITY 1 )
   configure_file( src/tenacity_config.h.in src/private/configunix.h )
endif()

# Generate a picture of module dependencies
string( JOIN "\n" GRAPH_EDGES ${GRAPH_EDGES} )
# Choose edge attributes making it easy to hover at either end of edge
# and see a tooltip describing the edge, in svg image
file( WRITE "${CMAKE_CURRENT_BINARY_DIR}/modules.dot" "digraph {
   graph [rankdir=LR] edge [dir=both,arrowtail=inv] \n"
   "${GRAPH_EDGES}"
   "\n}\n"
)
execute_process( COMMAND
   dot -O -Tsvg "${CMAKE_CURRENT_BINARY_DIR}/modules.dot" )

#
# Code signing
#

option( PERFORM_CODESIGN
        "Perform code signing during the install step. This only works on Windows and macOS. Might be broken."
        Off
)

cmake_dependent_option(
   PERFORM_NOTARIZATION
   "Perform notarization during the install step. This only works on macOS. Might be broken."
   Off
   "PERFORM_CODESIGN;APPLE"
   Off
)

if( PERFORM_CODESIGN )
   include( TenacityCodeSigning )
endif()


# Variables, that are common for all package targets

if( CMAKE_SYSTEM_NAME MATCHES "Windows" )
   include( TenacityInnoSetup )
endif()

# Uncomment what follows for symbol values.
#[[
   get_cmake_property( _variableNames VARIABLES )
   foreach( _variableName ${_variableNames} )
      message( STATUS "${_variableName}=${${_variableName}}" )
   endforeach()
#]]
#[[
   include( PrintProperties )
   print_properties( TARGET "wxWidgets" )
#]]

include( Package ) # do this last
