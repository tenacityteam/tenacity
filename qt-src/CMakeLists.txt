set( TARGET Tenacity )
set( TARGET_ROOT ${CMAKE_SOURCE_DIR}/qt-src )

include(CheckFunctionExists)

message( STATUS "========== Configuring ${TARGET} ==========" )

# Allow modules to link against the Tenacity executable
set( CMAKE_ENABLE_EXPORTS ON )

add_executable( ${TARGET} )
add_dependencies( ${TARGET} locale )
add_dependencies( ${TARGET} nyquist )
add_dependencies( ${TARGET} plug-ins )

def_vars()

# Add a target that will provide the git revision info
# whenever it changes.  (Must be done at build time, not
# configuration time.)
# if( DEFINED TENACITY_REV_LONG AND DEFINED AUDACITY_REV_TIME )
#    file( WRITE "${_PRVDIR}/RevisionIdent.h"
#       "#define REV_LONG \"${TENACITY_REV_LONG}\"\n#define REV_TIME \"${TENACITY_REV_TIME}\"\n"
#    )
# elseif( GIT_FOUND )
#    add_custom_target(
#       version
#       COMMAND
#          ${CMAKE_COMMAND} -D GIT="${GIT_EXECUTABLE}"
#                           -D "_PRVDIR=${_PRVDIR}"
#                           -P "cmake-proxies/cmake-modules/Version.cmake"
#       WORKING_DIRECTORY
#          ${CMAKE_SOURCE_DIR}
#    )

#    add_dependencies( ${TARGET} version )
# else()
#    # No Git installed and no version data is available.
#    # Generate an empty file and let AboutDialog do the rest
#    # WRITE, unlike TOUCH, will create any directories needed
#    # (see https://github.com/audacity/audacity/issues/2163)
#    file( WRITE "${_PRVDIR}/RevisionIdent.h" "" )
# endif()

# Handle Audio Units option
if( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
   option(
      use_audio_units
      "Use Audio Units plug-in support [on, off]"
      ON
   )
   set( USE_AUDIO_UNITS ${use_audio_units} CACHE INTERNAL "" )
endif()

# Detect, what platform we're on. Used for TenacityApp, VST effects, and Audio
# Unit effects.
#
# We always assume that GTK is used on platforms other than Windows and macOS,
# as there is no other implementation avalaible now
if( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
   set( wxIS_MAC on )
elseif( CMAKE_SYSTEM_NAME MATCHES "Windows" )
   set ( wxIS_WIN on )
else()
   set ( wxIS_GTK on)
endif()

#
#
#
list( APPEND SOURCES
    # Main source files
    main.cpp
    TenacityApp.cpp
    TenacityApp.h
    Forms/MainWindow/MainWindow.cpp
    Forms/MainWindow/MainWindow.h
    Forms/ControlToolbar/ControlToolbar.cpp
    Forms/ControlToolbar/ControlToolbar.h
)

#
#
#
list( APPEND INCLUDES
   PUBLIC
      ${_PRVDIR}
      ${CMAKE_SOURCE_DIR}/libraries
      ${TARGET_ROOT}
)

#
# Define resources
#
list( APPEND RESOURCES
   ../presets/EQDefaultCurves.xml
)

#
#
#
list( APPEND DEFINES
   PRIVATE
      BUILDING_AUDACITY
      CMAKE
      $<$<BOOL:${HAVE_LRINT}>:
         HAVE_LRINT
      >
      $<$<BOOL:${HAVE_LRINTF}>:
         HAVE_LRINTF
      >
      $<$<BOOL:${HAVE_MLOCK}>:
         HAVE_MLOCK
      >
      $<$<BOOL:${PA_USE_JACK}>:
         PA_USE_JACK
      >
      $<$<PLATFORM_ID:Windows>:
         _CRT_SECURE_NO_WARNINGS
         __STDC_CONSTANT_MACROS
         STRICT
      >
)

# We now use precompiled headers regardless, but only if ccache is not
# available and the user hasn't disabled it.
if( NOT CCACHE_PROGRAM AND NOT SCCACHE_PROGRAM )
   option(
      PCH
      "Use precompiled headers [yes, no]"
      YES
   )
endif()

tenacity_append_common_compiler_options( OPTIONS "${PCH}" )

list( APPEND LDFLAGS
   PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:/MANIFEST:NO>
      $<$<CXX_COMPILER_ID:GNU>:-Wl,--disable-new-dtags>
)

#
#
#
list( APPEND LIBRARIES
   PUBLIC
      ${CMAKE_REQUIRED_LIBRARIES}
      ZLIB::ZLIB
      mp3lame::mp3lame
      SndFile::sndfile
      PortAudio::PortAudio
      ${SQLite3_LIBRARIES}
      $<$<BOOL:${USE_FFMPEG}>:FFMPEG::FFMPEG>
      $<$<BOOL:${USE_LIBID3TAG}>:id3tag::id3tag>
      $<$<BOOL:${USE_LIBFLAC}>:FLAC++::FLAC++>
      $<$<BOOL:${USE_LIBMAD}>:MAD::MAD>
      $<$<BOOL:${USE_LIBMATROSKA}>:Matroska::matroska>
      $<$<BOOL:${USE_LIBOGG}>:Ogg::ogg>
      $<$<BOOL:${USE_LIBVORBIS}>:Vorbis::vorbis>
      $<$<BOOL:${USE_LIBVORBIS}>:Vorbis::vorbisenc>
      $<$<BOOL:${USE_LIBVORBIS}>:Vorbis::vorbisfile>
      $<$<BOOL:${USE_LIBTWOLAME}>:libtwolame::libtwolame>
      $<$<BOOL:${USE_LV2}>:lilv::lilv>
      $<$<BOOL:${USE_LV2}>:suil::suil>
      $<$<BOOL:${USE_MIDI}>:PortMidi::PortMidi>
      $<$<BOOL:${USE_MIDI}>:PortSMF::PortSMF>
      $<$<BOOL:${USE_SBSMS}>:sbsms::sbsms>
      $<$<BOOL:${USE_SOUNDTOUCH}>:SoundTouch::SoundTouch>
      $<$<BOOL:${USE_VAMP}>:VampHostSDK::VampHostSDK>
      $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD,NetBSD,CYGWIN>:${GLIB_LIBRARIES}>
      $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD,NetBSD,CYGWIN>:pthread>
      $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD,NetBSD,CYGWIN>:rt>
      Qt${QT_VERSION_MAJOR}::Core
      Qt${QT_VERSION_MAJOR}::Gui
      Qt${QT_VERSION_MAJOR}::Widgets
)

set( BUILDING_AUDACITY YES )
set( PKGLIBDIR "${_PKGLIB}" )
set( LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}" )
set( HAVE_GTK ${GTK_FOUND} )

# Do not define these for Windows or Mac until further testing
# can be done against ExportMP3.cpp.  If either are defined, the
# build will fail.
if( NOT CMAKE_SYSTEM_NAME MATCHES "Windows|Darwin" )
   if( FFMPEG_FOUND )
      set( DISABLE_DYNAMIC_LOADING_FFMPEG YES )
   endif()
endif()

if( CMAKE_SYSTEM_NAME MATCHES "Windows" )
   set_target_properties(
      ${TARGET}
      PROPERTIES
         # Make sure we're a GUI application and not a console tool
         WIN32_EXECUTABLE ON
   )

   # Define the Windows specific resources
   list( APPEND WIN_RESOURCES
      ../win/tenacity.rc
   )

   # Copy over the wxWidgets DLLs

   # Copy the required wxWidgets libs into the bundle
   add_custom_command(
      TARGET
         ${TARGET}
      COMMAND
         ${CMAKE_COMMAND} -D SRC="${_EXEDIR}/Tenacity.exe"
                          -D DST="${_EXEDIR}"
                          -D WXWIN="${_SHARED_PROXY_BASE_PATH}/$<CONFIG>/"
                          -P ${TENACITY_MODULE_PATH}/CopyLibs.cmake
      POST_BUILD
   )

   # Copy the VC runtime libraries as well
   add_custom_command(
      TARGET
         ${TARGET}
      COMMAND
         ${CMAKE_COMMAND} -E copy ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} ${_DEST}
      POST_BUILD
   )
elseif( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
   # Bug 2400 workaround
   #
   # Replaces the SDK version in the built executable with 10.13 to
   # prevent high CPU usage and slow drawing on Mojave or newer
   check_for_platform_version()
   if( PLATFORM_VERSION_SUPPORTED )
      list( APPEND LDFLAGS
         PRIVATE
            -Wl,-platform_version,macos,${MIN_MACOS_VERSION},${TARGET_MACOS_VERSION}
      )
   else()
      list( APPEND LDFLAGS
         PRIVATE
            -Wl,-sdk_version,10.13
      )
   endif()

   # Define Mac specific resources
   list( APPEND MAC_RESOURCES
      ../mac/Resources/Tenacity.icns
      ../mac/Resources/TenacityAIFF.icns
      ../mac/Resources/TenacityAU.icns
      ../mac/Resources/TenacityMP3.icns
      ../mac/Resources/TenacityOGG.icns
      ../mac/Resources/TenacityProject.icns
      ../mac/Resources/TenacityWAV.icns
   )

   set_target_properties(
      ${TARGET}
      PROPERTIES
         # Build a bundle
         MACOSX_BUNDLE ON
         # Add the resources
         RESOURCE "${MAC_RESOURCES};${RESOURCES}"
   )
   set_target_property_all( ${TARGET} RUNTIME_OUTPUT_DIRECTORY "${_DESTDIR}" )

   # These sources must be compiled as Objective C++ and excluded
   # from precompiled header usage
   set_source_files_properties(
      TenacityApp.mm
      KeyboardCapture.cpp
      effects/EffectUI.cpp
      effects/VST/VSTControlOSX.mm
      effects/audiounits/AUControl.mm
      menus/WindowMenus.cpp
      PROPERTIES
         COMPILE_FLAGS "-ObjC++"
         SKIP_PRECOMPILE_HEADERS YES
   )

   list( APPEND LIBRARIES
      PRIVATE
         "-framework Foundation"
         "-framework Cocoa"
         "-framework Carbon"
   )
   if(USE_AUDIO_UNITS)
      list(APPEND LIBRARIES
        PRIVATE
          "-framework AudioToolbox"
          "-framework AudioUnit"
          "-framework CoreAudioKit")
   endif()

   # Copy the required wxWidgets libs into the bundle
   add_custom_command(
      TARGET
         ${TARGET}
      COMMAND
         ${CMAKE_COMMAND} -D SRC="${_EXEDIR}/Tenacity"
                          -D DST="${_PKGLIB}"
                          -D WXWIN="${_SHARED_PROXY_BASE_PATH}/$<CONFIG>"
                          -P ${TENACITY_MODULE_PATH}/CopyLibs.cmake
      POST_BUILD
   )

   # Define the Wrapper target
   set( WRAPPER_ROOT "${TARGET_ROOT}/../mac" )
   set( WRAPPER_SOURCES "${WRAPPER_ROOT}/Wrapper.c" )

   add_executable( Wrapper "${WRAPPER_SOURCES}" )
   add_dependencies( "${TARGET}" Wrapper )

   set_target_property_all( "Wrapper" RUNTIME_OUTPUT_DIRECTORY "${_EXEDIR}" )
   organize_source( "${WRAPPER_ROOT}" "mac" "${WRAPPER_SOURCES}" )

elseif( CMAKE_SYSTEM_NAME MATCHES "Haiku" )
   set_target_property_all( ${TARGET} RUNTIME_OUTPUT_DIRECTORY "${_DEST}" )
else()
   set_target_property_all( ${TARGET} RUNTIME_OUTPUT_DIRECTORY "${_DEST}" )

   # Build the list of mimetypes
   #
   # (Don't use generator expressions since it will leave null/empty
   # entries in the list.)
   set( MIMETYPES
      application/x-audacity-project
      application/x-audacity-project+sqlite3
   )
   if( USE_FFMPEG )
      list( APPEND MIMETYPES
         audio/aac
         audio/ac3
         audio/mp4
         audio/x-ms-wma
         video/mpeg
      )
   endif()
   if( USE_LIBFLAC )
      list( APPEND MIMETYPES
         audio/flac
         audio/x-flac
      )
   endif()
   if( USE_LIBMAD )
      list( APPEND MIMETYPES
         audio/mpeg
      )
   endif()
   if( USE_LIBMATROSKA )
      list( APPEND MIMETYPES
         audio/matroska
         video/matroska
         audio/x-matroska
         video/x-matroska
      )
   endif()
   if( USE_SNDFILE )
      list( APPEND MIMETYPES
         audio/basic
         audio/x-aiff
         audio/x-wav
      )
   endif()
   if( USE_LIBOGG AND USE_LIBVORBIS )
      list( APPEND MIMETYPES
         application/ogg
         audio/x-vorbis+ogg
      )
   endif()

   # Create the desktop file
   configure_file( ${APP_NAME}.desktop.in ${_INTDIR}/${APP_NAME}.desktop )

   # Copy the required wxWidgets libs into the bundle
   add_custom_command(
      TARGET
         ${TARGET}
      COMMAND
         ${CMAKE_COMMAND} -D SRC="${_EXEDIR}/tenacity"
                          -D DST="${_DEST}/${_PKGLIB}"
                          -D WXWIN="${_SHARED_PROXY_BASE_PATH}/$<CONFIG>"
                          -P ${TENACITY_MODULE_PATH}/CopyLibs.cmake
      POST_BUILD
   )

   add_executable(findlib ../linux/findlib.c)
   target_link_libraries(findlib ${CMAKE_DL_LIBS})
   set_target_property_all( findlib SKIP_BUILD_RPATH On )
endif()

set_target_property_all( ${TARGET} RUNTIME_OUTPUT_NAME ${APP_NAME} )

organize_source( "${TARGET_ROOT}/../presets" "presets" "${RESOURCES}" )
organize_source( "${TARGET_ROOT}" "src" "${SOURCES}" )
organize_source( "${TARGET_ROOT}/../mac/Resources" "mac" "${MAC_RESOURCES}" )
organize_source( "${TARGET_ROOT}/../win" "win" "${WIN_RESOURCES}" )

#
# Get those pesky CMake files out of the way (on Windows)
#
source_group(
    TREE
        "${_INTDIR}"
    PREFIX
        "CMake"
    FILES
        ${_INTDIR}/CMakeFiles/Audacity.dir/cmake_pch.cxx
        ${_INTDIR}/CMakeFiles/Audacity.dir/cmake_pch.hxx
)

# Define TENACITY_DLL_API
import_symbol_define( import_symbol TENACITY_DLL )
export_symbol_define( export_symbol TENACITY_DLL )
list( APPEND DEFINES PRIVATE "${export_symbol}" INTERFACE "${import_symbol}" )

# see AudioIO.cpp
check_function_exists(clock_gettime HAVE_CLOCK_GETTIME)
if(HAVE_CLOCK_GETTIME)
   list( APPEND DEFINES HAVE_CLOCK_GETTIME )
endif()

target_sources( ${TARGET} PRIVATE ${SOURCES} ${RESOURCES} ${MAC_RESOURCES} ${WIN_RESOURCES} )
target_compile_definitions( ${TARGET} PRIVATE ${DEFINES} )
target_compile_options( ${TARGET} PRIVATE ${OPTIONS} )
target_include_directories( ${TARGET} PRIVATE ${INCLUDES} )
target_link_options( ${TARGET} PRIVATE ${LDFLAGS} )
target_link_libraries( ${TARGET} PUBLIC ${TENACITY_LIBRARIES} )
target_link_libraries( ${TARGET} ${LIBRARIES} )

if( NOT CCACHE_PROGRAM AND NOT SCCACHE_PROGRAM )
   if( PCH )
      message( STATUS "Using precompiled headers" )
      target_precompile_headers( ${TARGET} PRIVATE
         $<$<PLATFORM_ID:Windows>:${CMAKE_BINARY_DIR}/src/private/configwin.h>
         $<$<PLATFORM_ID:Darwin>:${CMAKE_BINARY_DIR}/src/private/configmac.h>
         $<$<NOT:$<PLATFORM_ID:Windows,Darwin>>:${CMAKE_BINARY_DIR}/src/private/configunix.h>
         TenacityHeaders.h
      )
   else()
      message( STATUS "Not using precompiled headers" )
   endif()
endif()

if( "${CMAKE_GENERATOR}" MATCHES "Xcode|Visual Studio*" )
   install(
      DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/"
      DESTINATION "."
      USE_SOURCE_PERMISSIONS
      PATTERN "*.pdb" EXCLUDE
      PATTERN "*.ilk" EXCLUDE
      PATTERN "*.dSYM" EXCLUDE
   )
else()
   if( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
      install( TARGETS ${TARGET}
               DESTINATION "."
               RESOURCE DESTINATION "${_APPDIR}/Resources" )
   elseif( CMAKE_SYSTEM_NAME MATCHES "Haiku" )
      install( TARGETS ${TARGET} RUNTIME )
      install( DIRECTORY "${_DEST}/${_LIBDIR}/"
               DESTINATION "${_LIBDIR}"
               USE_SOURCE_PERMISSIONS
               FILES_MATCHING PATTERN "*.so*" )
      install( FILES "${CMAKE_SOURCE_DIR}/LICENSE.txt" "${CMAKE_SOURCE_DIR}/README.md"
               DESTINATION "${_DATADIR}/doc/${APP_NAME}" )
      install( FILES "${CMAKE_SOURCE_DIR}/presets/EQDefaultCurves.xml"
               DESTINATION "${_PKGDATA}" )
   else()
      install( TARGETS ${TARGET} RUNTIME )
      install( DIRECTORY "${_DEST}/${_LIBDIR}/"
               DESTINATION "${_LIBDIR}"
               USE_SOURCE_PERMISSIONS
               FILES_MATCHING PATTERN "*.so*" )
      install( FILES "${_INTDIR}/${APP_NAME}.desktop"
               DESTINATION "${_DATADIR}/applications" )
      install( FILES "${CMAKE_SOURCE_DIR}/LICENSE.txt" "${CMAKE_SOURCE_DIR}/README.md"
               DESTINATION "${_DATADIR}/doc/${APP_NAME}" )
      install( FILES "${_SRCDIR}/tenacity.xml"
               DESTINATION "${_DATADIR}/mime/packages" )
      install( FILES "${CMAKE_SOURCE_DIR}/presets/EQDefaultCurves.xml"
               DESTINATION "${_PKGDATA}" )
      install( PROGRAMS "${PROJECT_SOURCE_DIR}/linux/tenacity.sh"
               DESTINATION "."
               RENAME "tenacity" )
   endif()
endif()

# collect dependency information for third party libraries
list( APPEND GRAPH_EDGES "Tenacity [shape=house]" )
foreach( LIBRARY ${LIBRARIES} )
   if (NOT LIBRARY MATCHES "PUBLIC|PRIVATE|INTERFACE")
      canonicalize_node_name(LIBRARY "${LIBRARY}")
      list( APPEND GRAPH_EDGES "\"${TARGET}\" -> \"${LIBRARY}\"" )
   endif()
endforeach()

#propagate collected edgets to root CMakeLists.txt
set( GRAPH_EDGES "${GRAPH_EDGES}" PARENT_SCOPE )
