# Make Tenacity.app self-contained so it runs on Macs other than the one
# that built it.
#
# Every Mach-O file in the bundle (the executable, the libraries in
# Contents/Frameworks and the modules in Contents/modules) is checked with
# otool. Third-party libraries it needs that aren't in the bundle yet are
# copied into Contents/Frameworks and checked in turn, absolute library
# references are rewritten to @rpath, absolute LC_RPATH entries pointing
# into the build tree, vcpkg or Homebrew are removed, and the executable
# and modules get @loader_path/../Frameworks if missing. Anything modified is
# ad-hoc signed again; TENACITY-MACOS-BUNDLE signs the whole bundle after.
#
# Defines required:
#
# BUNDLE       path to Tenacity.app
# SEARCH_DIRS  '|' separated directories to find @rpath libraries in
#

cmake_policy( VERSION 3.25 )

if( NOT IS_DIRECTORY "${BUNDLE}" )
   message( FATAL_ERROR "BundleMacDeps: no bundle at '${BUNDLE}'" )
endif()

set( FRAMEWORKS "${BUNDLE}/Contents/Frameworks" )
set( BUNDLE_RPATH "@loader_path/../Frameworks" )
string( REPLACE "|" ";" SEARCH_DIRS "${SEARCH_DIRS}" )
file( MAKE_DIRECTORY "${FRAMEWORKS}" )

function( run_lines out )
   execute_process(
      COMMAND ${ARGN}
      OUTPUT_VARIABLE output
      ERROR_VARIABLE error
      RESULT_VARIABLE result
   )
   if( result )
      string( JOIN " " command ${ARGN} )
      message( FATAL_ERROR "BundleMacDeps: '${command}' failed:\n${error}" )
   endif()
   string( REPLACE "\n" ";" output "${output}" )
   set( ${out} "${output}" PARENT_SCOPE )
endfunction()

# run_lines for commands whose output we don't need; still fails the build
# on error
function( run_lines_silent )
   run_lines( unused ${ARGN} )
endfunction()

# Libraries the file links to, and for a library its own install name too
function( get_deps file out )
   set( dep_line "^[ \t]+(.+) \\(compatibility version .*$" )
   run_lines( lines otool -L "${file}" )
   list( FILTER lines INCLUDE REGEX "${dep_line}" )
   list( TRANSFORM lines REPLACE "${dep_line}" "\\1" )
   set( ${out} "${lines}" PARENT_SCOPE )
endfunction()

# LC_RPATH is the only load command otool -l prints a path line for
function( get_rpaths file out )
   set( path_line "^ *path (.+) \\(offset [0-9]+\\)$" )
   run_lines( lines otool -l "${file}" )
   list( FILTER lines INCLUDE REGEX "${path_line}" )
   list( TRANSFORM lines REPLACE "${path_line}" "\\1" )
   list( REMOVE_DUPLICATES lines )
   set( ${out} "${lines}" PARENT_SCOPE )
endfunction()

# Copy library ${name} into Contents/Frameworks, from ${hint} if given or
# else from SEARCH_DIRS, and fix up the copy too. Libraries we built
# (lib-*.dylib) are already there and aren't in SEARCH_DIRS.
function( bundle_lib name hint needed_by )
   set( dst "${FRAMEWORKS}/${name}" )

   if( hint )
      set( src "${hint}" )
   else()
      # find_file doesn't search if src is already set, and the caller's
      # variables are visible here when fix_file recurses back into us
      unset( src )
      find_file( src "${name}" PATHS ${SEARCH_DIRS} NO_DEFAULT_PATH NO_CACHE )
   endif()

   if( NOT EXISTS "${src}" )
      if( EXISTS "${dst}" )
         return()
      endif()
      message( FATAL_ERROR
         "BundleMacDeps: can't find ${name}, needed by ${needed_by}" )
   endif()

   file( REAL_PATH "${src}" src )
   if( EXISTS "${dst}" AND NOT "${src}" IS_NEWER_THAN "${dst}" )
      return()
   endif()

   message( STATUS "Bundling ${src} as Frameworks/${name}" )
   file( COPY_FILE "${src}" "${dst}" )
   file( CHMOD "${dst}"
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                  GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE )
   run_lines_silent( install_name_tool -id "@rpath/${name}" "${dst}" )
   fix_file( "${dst}" )
endfunction()

function( fix_file file )
   # Global since every call gets its own scope, and so libraries that
   # need each other don't recurse forever
   get_property( done GLOBAL PROPERTY BUNDLE_DONE )
   if( file IN_LIST done )
      return()
   endif()
   set_property( GLOBAL APPEND PROPERTY BUNDLE_DONE "${file}" )

   set( args )

   get_filename_component( self "${file}" NAME )
   get_deps( "${file}" deps )
   foreach( dep ${deps} )
      get_filename_component( name "${dep}" NAME )
      if( name STREQUAL self OR dep MATCHES "^/(usr/lib|System)/" )
         continue()
      endif()
      # @rpath, @loader_path and @executable_path all end up meaning
      # Contents/Frameworks once the library is copied there
      if( dep MATCHES "^/" )
         bundle_lib( "${name}" "${dep}" "${file}" )
         list( APPEND args -change "${dep}" "@rpath/${name}" )
      else()
         bundle_lib( "${name}" "" "${file}" )
      endif()
   endforeach()

   get_rpaths( "${file}" rpaths )
   set( absolute_rpaths ${rpaths} )
   list( FILTER absolute_rpaths INCLUDE REGEX "^/" )
   foreach( rpath ${absolute_rpaths} )
      list( APPEND args -delete_rpath "${rpath}" )
   endforeach()

   # Libraries in Contents/Frameworks are only ever loaded by the
   # executable or a module, whose LC_RPATH dyld searches as well, so
   # don't add to third-party ones that may lack the header padding
   get_filename_component( dir "${file}" DIRECTORY )
   if( NOT dir STREQUAL FRAMEWORKS AND NOT BUNDLE_RPATH IN_LIST rpaths )
      list( APPEND args -add_rpath "${BUNDLE_RPATH}" )
   endif()

   if( args )
      message( STATUS "Fixing library paths in ${file}" )
      run_lines_silent( install_name_tool ${args} "${file}" )
      run_lines_silent( codesign --force --sign - "${file}" )
   endif()

   get_rpaths( "${file}" rpaths )
   list( FILTER rpaths INCLUDE REGEX "^/" )
   if( rpaths )
      message( FATAL_ERROR "BundleMacDeps: ${file} still has LC_RPATH ${rpaths}" )
   endif()
endfunction()

file( GLOB files
   "${BUNDLE}/Contents/MacOS/*"
   "${FRAMEWORKS}/*.dylib"
   "${BUNDLE}/Contents/modules/*.so"
)
foreach( file IN LISTS files )
   fix_file( "${file}" )
endforeach()
