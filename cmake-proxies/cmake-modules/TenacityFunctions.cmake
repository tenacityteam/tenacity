#
# A collection of functions and macros
#

# Defines several useful directory paths for the active context.
macro( def_vars )
   set( _SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}" )
   set( _INTDIR "${CMAKE_CURRENT_BINARY_DIR}" )
   set( _PRVDIR "${CMAKE_CURRENT_BINARY_DIR}/private" )
   set( _PUBDIR "${CMAKE_CURRENT_BINARY_DIR}/public" )
endmacro()

# Helper to organize sources into folders for the IDEs
macro( organize_source root prefix sources )
   set( cleaned )
   foreach( source ${sources} )
      # Remove generator expressions
      string( REGEX REPLACE ".*>:(.*)>*" "\\1" source "${source}" )
      string( REPLACE ">" "" source "${source}" )

      # Remove keywords
      string( REGEX REPLACE "^[A-Z]+$" "" source "${source}" )

      # Add to cleaned
      list( APPEND cleaned "${source}" )
   endforeach()

   # Define the source groups
   if( "${prefix}" STREQUAL "" )
      source_group( TREE "${root}" FILES ${cleaned} )
   else()
      source_group( TREE "${root}" PREFIX ${prefix} FILES ${cleaned} )
   endif()
endmacro()

# Given a directory, recurse to all defined subdirectories and assign
# the given folder name to all of the targets found.
function( set_dir_folder dir folder)
   get_property( subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES )
   foreach( sub ${subdirs} )
      set_dir_folder( "${sub}" "${folder}" )
   endforeach()

   get_property( targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS )
   foreach( target ${targets} )
      get_target_property( type "${target}" TYPE )
      if( NOT "${type}" STREQUAL "INTERFACE_LIBRARY" )
         set_target_properties( ${target} PROPERTIES FOLDER ${folder} )
      endif()
   endforeach()
endfunction()

# Helper to retrieve the settings returned from pkg_check_modules()
macro( get_package_interface package )
   set( INCLUDES
      ${${package}_INCLUDE_DIRS}
   )

   set( LINKDIRS
      ${${package}_LIBDIR}
   )

   # We resolve the full path of each library to ensure the
   # correct one is referenced while linking
   foreach( lib ${${package}_LIBRARIES} )
      find_library( LIB_${lib} ${lib} HINTS ${LINKDIRS} )
      list( APPEND LIBRARIES ${LIB_${lib}} )
   endforeach()
endmacro()

# Set the cache and context value
macro( set_cache_value var value )
   set( ${var} "${value}" )
   set_property( CACHE ${var} PROPERTY VALUE "${value}" )
endmacro()

# Set a CMake variable to the value of the corresponding environment variable
# if the CMake variable is not already defined. Any addition arguments after
# the variable name are passed through to set().
macro( set_from_env var )
   if( NOT DEFINED ${var} AND NOT "$ENV{${var}}" STREQUAL "" )
      set( ${var} "$ENV{${var}}" ${ARGN} ) # pass additional args (e.g. CACHE)
   endif()
endmacro()

# Set the given property and its config specific brethren to the same value
function( set_target_property_all target property value )
   set_target_properties( "${target}" PROPERTIES "${property}" "${value}" )
   foreach( type ${CMAKE_CONFIGURATION_TYPES} )
      string( TOUPPER "${property}_${type}" prop )
      set_target_properties( "${target}" PROPERTIES "${prop}" "${value}" )
   endforeach()
endfunction()

# Taken from wxWidgets and modified for Audacity
#
# cmd_option(<name> <desc> [default] [STRINGS strings])
# The default is ON if third parameter isn't specified
function( cmd_option name desc )
   cmake_parse_arguments( OPTION "" "" "STRINGS" ${ARGN} )

   if( ARGC EQUAL 2 )
      if( OPTION_STRINGS )
         list( GET OPTION_STRINGS 1 default )
      else()
         set( default ON )
      endif()
   else()
      set( default ${OPTION_UNPARSED_ARGUMENTS} )
   endif()

   if( OPTION_STRINGS )
      set( cache_type STRING )
   else()
      set( cache_type BOOL )
   endif()

   set( ${name} "${default}" CACHE ${cache_type} "${desc}" )
   if( OPTION_STRINGS )
      set_property( CACHE ${name} PROPERTY STRINGS ${OPTION_STRINGS} )

      # Check valid value
      set( value_is_valid FALSE )
      set( avail_values )
      foreach( opt ${OPTION_STRINGS} )
         if( ${name} STREQUAL opt )
            set( value_is_valid TRUE )
            break()
         endif()
         string( APPEND avail_values " ${opt}" )
      endforeach()
      if( NOT value_is_valid )
         message( FATAL_ERROR "Invalid value \"${${name}}\" for option ${name}. Valid values are: ${avail_values}" )
      endif()
   endif()

   set( ${name} "${${name}}" PARENT_SCOPE )
endfunction()

# Determines if the linker supports the "-platform_version" argument
# on macOS.
macro( check_for_platform_version )
   if( NOT DEFINED LINKER_SUPPORTS_PLATFORM_VERSION )
      execute_process(
         COMMAND
            ld -platform_version macos 1.1 1.1
         ERROR_VARIABLE
            error
      )

      if( error MATCHES ".*unknown option.*" )
         set( PLATFORM_VERSION_SUPPORTED no CACHE INTERNAL "" )
      else()
         set( PLATFORM_VERSION_SUPPORTED yes CACHE INTERNAL "" )
      endif()
   endif()
endmacro()

# To be used to compile all C++ in the application and modules
function( tenacity_append_common_compiler_options var use_pch )
   if( NOT use_pch )
      list( APPEND ${var}
         PRIVATE
            # include the correct config file; give absolute path to it, so
            # that this works whether in src, modules, libraries
            $<$<CXX_COMPILER_ID:MSVC>:/FI${CMAKE_BINARY_DIR}/src/private/config.h>
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-include ${CMAKE_BINARY_DIR}/src/private/config.h>
      )
   endif()
   list( APPEND ${var}
         -DTENACITY_VERSION=${TENACITY_VERSION}
         -DTENACITY_RELEASE=${TENACITY_RELEASE}
         -DTENACITY_REVISION=${TENACITY_REVISION}

         # Version string for visual display
         -DTENACITY_VERSION_STRING=L"${TENACITY_VERSION}.${TENACITY_RELEASE}.${TENACITY_REVISION}${GIT_VERSION_TAG}"

         # This value is used in the resource compiler for Windows
         -DTENACITY_FILE_VERSION=L"${TENACITY_VERSION},${TENACITY_RELEASE},${TENACITY_REVISION}"

         # Reviewed, certified, non-leaky uses of NEW that immediately entrust
	      # their results to RAII objects.
         # You may use it in NEW code when constructing a wxWindow subclass
	      # with non-NULL parent window.
         # You may use it in NEW code when the NEW expression is the
	      # constructor argument for a standard smart
         # pointer like std::unique_ptr or std::shared_ptr.
         -Dsafenew=new

         $<$<CXX_COMPILER_ID:MSVC>:/permissive->
         $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wno-underaligned-exception-object>
         $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Werror=return-type>
         $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Werror=dangling-else>
         $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Werror=return-stack-address>
	      # Yes, CMake will change -D to /D as needed for Windows:
         -DWXINTL_NO_GETTEXT_MACRO
         $<$<PLATFORM_ID:Windows>:-D_USE_MATH_DEFINES>
         $<$<PLATFORM_ID:Windows>:-DNOMINMAX>

         # Define/undefine _DEBUG
	 # Yes, -U to /U too as needed for Windows:
	 $<IF:$<CONFIG:Debug>,-D_DEBUG=1,-U_DEBUG>
   )   
   # Definitions controlled by the TENACITY_BUILD_LEVEL switch
   if( TENACITY_BUILD_LEVEL EQUAL 0 )
      list( APPEND ${var} -DIS_ALPHA -DUSE_ALPHA_MANUAL )
   elseif( TENACITY_BUILD_LEVEL EQUAL 1 )
      list( APPEND ${var} -DIS_BETA -DUSE_ALPHA_MANUAL )
   else()
      list( APPEND ${var} -DIS_RELEASE )
   endif()

   set( ${var} "${${var}}" PARENT_SCOPE )
endfunction()

function( import_export_symbol var module_name )
   # compute, e.g. "TRACK_UI_API" from module name "mod-track-ui"
   string( REGEX REPLACE "^mod-" "" symbol "${module_name}" )
   string( REGEX REPLACE "^lib-" "" symbol "${symbol}" )
   string( TOUPPER "${symbol}" symbol )
   string( REPLACE "-" "_" symbol "${symbol}" )
   string( APPEND symbol "_API" )
   set( "${var}" "${symbol}" PARENT_SCOPE )
endfunction()

function( import_symbol_define var module_name )
   import_export_symbol( symbol "${module_name}" )
   if( CMAKE_SYSTEM_NAME MATCHES "Windows" )
      set( value "_declspec(dllimport)" )
   elseif( HAVE_VISIBILITY )
      set( value "__attribute__((visibility(\"default\")))" )
   else()
      set( value "" )
   endif()
   set( "${var}" "${symbol}=${value}" PARENT_SCOPE )
endfunction()

function( export_symbol_define var module_name )
   import_export_symbol( symbol "${module_name}" )
   if( CMAKE_SYSTEM_NAME MATCHES "Windows" )
      set( value "_declspec(dllexport)" )
   elseif( HAVE_VISIBILITY )
      set( value "__attribute__((visibility(\"default\")))" )
   else()
      set( value "" )
   endif()
   set( "${var}" "${symbol}=${value}" PARENT_SCOPE )
endfunction()

# shorten a target name for purposes of generating a dependency graph picture
function( canonicalize_node_name var node )
   # strip generator expressions
   string( REGEX REPLACE ".*>.*:(.*)>" "\\1" node "${node}" )
   # omit the "-interface" for alias targets to modules
   string( REGEX REPLACE "-interface\$" "" node "${node}"  )
   # shorten names of standard libraries or Apple frameworks
   string( REGEX REPLACE "^-(l|framework )" "" node "${node}" )
   # shorten paths
   get_filename_component( node "${node}" NAME_WE )
   set( "${var}" "${node}" PARENT_SCOPE )
endfunction()

function( tenacity_module_fn NAME SOURCES IMPORT_TARGETS
   ADDITIONAL_DEFINES ADDITIONAL_LIBRARIES LIBTYPE )

   set( TARGET ${NAME} )
   set( TARGET_ROOT ${CMAKE_CURRENT_SOURCE_DIR} )

   message( STATUS "========== Configuring ${TARGET} ==========" )

   def_vars()

   if (LIBTYPE STREQUAL "MODULE" AND CMAKE_SYSTEM_NAME MATCHES "Windows")
      set( REAL_LIBTYPE SHARED )
   else()
      set( REAL_LIBTYPE "${LIBTYPE}" )
   endif()
   add_library( ${TARGET} ${REAL_LIBTYPE} )

   # Manual propagation seems to be necessary from
   # interface libraries -- just doing target_link_libraries naming them
   # doesn't work as promised

   # compute INCLUDES
   set( INCLUDES )
   list( APPEND INCLUDES PUBLIC ${TARGET_ROOT} )

   # compute DEFINES
   set( DEFINES )
   list( APPEND DEFINES ${ADDITIONAL_DEFINES} )

   # send the file to the proper place in the build tree, by setting the
   # appropriate property for the platform
   if (CMAKE_SYSTEM_NAME MATCHES "Windows")
      set( DIRECTORY_PROPERTY RUNTIME_OUTPUT_DIRECTORY )
   else ()
      set( DIRECTORY_PROPERTY LIBRARY_OUTPUT_DIRECTORY )
   endif ()

   if (LIBTYPE STREQUAL "MODULE")
      set( ATTRIBUTES "shape=box" )
      set_target_property_all( ${TARGET} ${DIRECTORY_PROPERTY} "${_MODDIR}" )
      set_target_properties( ${TARGET}
         PROPERTIES
            PREFIX ""
            FOLDER "modules" # for IDE organization
      )
      if( CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin" )
         add_custom_command(
	    TARGET ${TARGET}
            COMMAND ${CMAKE_COMMAND}
	       -D SRC="${_MODDIR}/${TARGET}.so"
               -D WXWIN="${_SHARED_PROXY_BASE_PATH}/$<CONFIG>"
               -P ${TENACITY_MODULE_PATH}/CopyLibs.cmake
            POST_BUILD )
      endif()
   else()
      set( ATTRIBUTES "shape=octagon" )
      set_target_property_all( ${TARGET} ${DIRECTORY_PROPERTY} "${_SHARED_PROXY_PATH}" )
      set_target_properties( ${TARGET}
         PROPERTIES
            PREFIX ""
            FOLDER "libraries" # for IDE organization
            INSTALL_NAME_DIR ""
            BUILD_WITH_INSTALL_NAME_DIR YES
      )
   endif()

   if( "wxBase" IN_LIST IMPORT_TARGETS OR "wxwidgets::base" IN_LIST IMPORT_TARGETS )
      string( APPEND ATTRIBUTES " style=filled" )
   endif()

   export_symbol_define( export_symbol "${TARGET}" )
   import_symbol_define( import_symbol "${TARGET}" )

   list( APPEND DEFINES
      PRIVATE "${export_symbol}"
      INTERFACE "${import_symbol}"
   )

   set( LOPTS
      PRIVATE
         $<$<PLATFORM_ID:Darwin>:-undefined dynamic_lookup>
   )

   # compute LIBRARIES
   set( LIBRARIES )
   
   foreach( IMPORT ${IMPORT_TARGETS} )
      list( APPEND LIBRARIES "${IMPORT}" )
   endforeach()

   list( APPEND LIBRARIES ${ADDITIONAL_LIBRARIES} )

#   list( TRANSFORM SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/" )

   # Compute compilation options.
   # Perhaps a another function argument in future to customize this too.
   set( OPTIONS )
   tenacity_append_common_compiler_options( OPTIONS NO )

   organize_source( "${TARGET_ROOT}" "" "${SOURCES}" )
   target_sources( ${TARGET} PRIVATE ${SOURCES} )
   target_compile_definitions( ${TARGET} PRIVATE ${DEFINES} )
   target_compile_options( ${TARGET} ${OPTIONS} )
   target_include_directories( ${TARGET} PUBLIC ${TARGET_ROOT} )

   target_link_options( ${TARGET} PRIVATE ${LOPTS} )
   target_link_libraries( ${TARGET} PUBLIC ${LIBRARIES} )

   if( NOT CMAKE_SYSTEM_NAME MATCHES "Windows" )
      add_custom_command(
         TARGET "${TARGET}"
         POST_BUILD
         COMMAND $<IF:$<CONFIG:Debug>,echo,strip> -x $<TARGET_FILE:${TARGET}>
      )
   endif()

   if( NOT REAL_LIBTYPE STREQUAL "MODULE" )
      if( CMAKE_SYSTEM_NAME MATCHES "Windows" )
         set( REQUIRED_LOCATION "${_EXEDIR}" )
      elseif( CMAKE_SYSTEM_NAME MATCHES "Darwin")
         set( REQUIRED_LOCATION "${_PKGLIB}" )
      else()
         set( REQUIRED_LOCATION "${_DEST}/${_PKGLIB}" )
      endif()

      add_custom_command(TARGET ${TARGET} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy 
            "$<TARGET_FILE:${TARGET}>" 
            "${REQUIRED_LOCATION}/$<TARGET_FILE_NAME:${TARGET}>"
      )
   endif()

   # define an additional interface library target
   set(INTERFACE_TARGET "${TARGET}-interface")
   if (NOT REAL_LIBTYPE STREQUAL "MODULE")
      add_library("${INTERFACE_TARGET}" ALIAS "${TARGET}")
   else()
      add_library("${INTERFACE_TARGET}" INTERFACE)
      foreach(PROP
         INTERFACE_INCLUDE_DIRECTORIES
         INTERFACE_COMPILE_DEFINITIONS
         INTERFACE_LINK_LIBRARIES
      )
         get_target_property( PROPS "${TARGET}" "${PROP}" )
         if (PROPS)
            set_target_properties(
               "${INTERFACE_TARGET}"
               PROPERTIES "${PROP}" "${PROPS}" )
         endif()
      endforeach()
   endif()

   # collect dependency information
   list( APPEND GRAPH_EDGES "\"${TARGET}\" [${ATTRIBUTES}]" )
   if (NOT LIBTYPE STREQUAL "MODULE")
      list( APPEND GRAPH_EDGES "\"Audacity\" -> \"${TARGET}\"" )
   endif ()
   set(ACCESS PUBLIC PRIVATE INTERFACE)
   foreach( IMPORT ${IMPORT_TARGETS} )
      if(IMPORT IN_LIST ACCESS)
         continue()
      endif()
      canonicalize_node_name(IMPORT "${IMPORT}")
      list( APPEND GRAPH_EDGES "\"${TARGET}\" -> \"${IMPORT}\"" )
   endforeach()
   set( GRAPH_EDGES "${GRAPH_EDGES}" PARENT_SCOPE )
endfunction()

function( append_node_attributes var target )
   get_target_property( dependencies ${target} AUDACITY_GRAPH_DEPENDENCIES )
   set( color "lightpink" )
   if( NOT "wxWidgets::wxWdgets" IN_LIST dependencies )
      # Toolkit neutral targets
      set( color "lightgreen" )
      get_target_property(type ${target} TYPE)
      if (NOT ${type} STREQUAL "INTERFACE_LIBRARY")
         # Enforce usage of only a subset of wxBase that excludes the event loop
         # apply_wxbase_restrictions( ${target} )
      endif()
   endif()
   string( APPEND "${var}" " style=filled fillcolor=${color}" )
   set( "${var}" "${${var}}" PARENT_SCOPE)
endfunction()

function (propagate_interesting_dependencies target direct_dependencies )
   # use a custom target attribute to propagate information up the graph about
   # some interesting transitive dependencies
   set( interesting_dependencies )
   foreach( direct_dependency ${direct_dependencies} )
      if ( NOT TARGET "${direct_dependency}" )
         continue()
      endif ()
      get_target_property( more_dependencies
         ${direct_dependency} AUDACITY_GRAPH_DEPENDENCIES )
      if ( more_dependencies )
         list( APPEND interesting_dependencies ${more_dependencies} )
      endif ()
      foreach( special_dependency
         "wxWidgets::wxWidgets"
      )
         if( special_dependency STREQUAL direct_dependency )
            list( APPEND interesting_dependencies "${special_dependency}" )
         endif()
      endforeach()
   endforeach()
   list( REMOVE_DUPLICATES interesting_dependencies )
   set_target_properties( ${target} PROPERTIES
      AUDACITY_GRAPH_DEPENDENCIES "${interesting_dependencies}" )
endfunction()

# Set up for defining a module target.
# All modules depend on the application.
# Pass a name and sources, and a list of other targets.
# Use the interface compile definitions and include directories of the
# other targets, and link to them.
# More defines, and more target libraries (maybe generator expressions)
# may be given too.
macro( tenacity_module NAME SOURCES IMPORT_TARGETS
   ADDITIONAL_DEFINES ADDITIONAL_LIBRARIES )
   # The extra indirection of a function call from this macro, and
   # re-assignment of GRAPH_EDGES, is so that a module definition may
   # call this macro, and it will (correctly) collect edges for the
   # CMakeLists.txt in the directory above it; but otherwise we take
   # advantage of function scoping of variables.
   tenacity_module_fn(
      "${NAME}"
      "${SOURCES}"
      "${IMPORT_TARGETS}"
      "${ADDITIONAL_DEFINES}"
      "${ADDITIONAL_LIBRARIES}"
      "MODULE"
   )
   set( GRAPH_EDGES "${GRAPH_EDGES}" PARENT_SCOPE )
endmacro()

# Set up for defining a library target.
# The application depends on all libraries.
# Pass a name and sources, and a list of other targets.
# Use the interface compile definitions and include directories of the
# other targets, and link to them.
# More defines, and more target libraries (maybe generator expressions)
# may be given too.
macro( tenacity_library NAME SOURCES IMPORT_TARGETS
   ADDITIONAL_DEFINES ADDITIONAL_LIBRARIES )
   # ditto comment in the previous macro
   if (${BUILD_STATIC_LIBS})
      tenacity_module_fn(
         "${NAME}"
         "${SOURCES}"
         "${IMPORT_TARGETS}"
         "${ADDITIONAL_DEFINES}"
         "${ADDITIONAL_LIBRARIES}"
         "STATIC"
      )
   else()
      tenacity_module_fn(
         "${NAME}"
         "${SOURCES}"
         "${IMPORT_TARGETS}"
         "${ADDITIONAL_DEFINES}"
         "${ADDITIONAL_LIBRARIES}"
         "SHARED"
      )
   endif()
   set( GRAPH_EDGES "${GRAPH_EDGES}" PARENT_SCOPE )
   # Collect list of libraries for the executable to declare dependency on
   list( APPEND TENACITY_LIBRARIES "${NAME}" )
   set( TENACITY_LIBRARIES "${TENACITY_LIBRARIES}" PARENT_SCOPE )
endmacro()

function ( make_interface_alias TARGET REAL_LIBTYTPE )
   set(INTERFACE_TARGET "${TARGET}-interface")
   if (NOT REAL_LIBTYPE STREQUAL "MODULE")
      add_library("${INTERFACE_TARGET}" ALIAS "${TARGET}")
   else()
      add_library("${INTERFACE_TARGET}" INTERFACE)
      foreach(PROP
         INTERFACE_INCLUDE_DIRECTORIES
         INTERFACE_COMPILE_DEFINITIONS
         INTERFACE_LINK_LIBRARIES
	 AUDACITY_GRAPH_DEPENDENCIES
      )
         get_target_property( PROPS "${TARGET}" "${PROP}" )
         if (PROPS)
            set_target_properties(
               "${INTERFACE_TARGET}"
               PROPERTIES "${PROP}" "${PROPS}" )
         endif()
      endforeach()
   endif()
endfunction()

function(make_interface_library
   new  # name for new target
   old   # existing library target
)
   add_library(${new} INTERFACE)
   copy_target_properties(${old} ${new}
      INTERFACE_COMPILE_DEFINITIONS
      INTERFACE_COMPILE_OPTIONS
      INTERFACE_INCLUDE_DIRECTORIES
      INTERFACE_LINK_DIRECTORIES
      INTERFACE_LINK_LIBRARIES)
endfunction()

function(fix_bundle target_name)
   if (NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
      return()
   endif()

   add_custom_command(
      TARGET ${target_name}
      POST_BUILD
      COMMAND
         ${PYTHON}
         ${CMAKE_SOURCE_DIR}/scripts/build/macOS/fix_bundle.py
         $<TARGET_FILE:${target_name}>
	 -config=$<CONFIG>
   )
endfunction()

# Copy named properties from one target to another
function(copy_target_properties
  src  # target
  dest # target
  # prop1 prop2...
)
   foreach(property ${ARGN})
      get_target_property(value ${src} ${property})
      if(value)
         set_target_properties(${dest} PROPERTIES ${property} "${value}")
      endif()
   endforeach()
endfunction()

# A special macro for header only libraries

macro( tenacity_header_only_library NAME SOURCES IMPORT_TARGETS
   ADDITIONAL_DEFINES )
   # ditto comment in the previous macro
   add_library( ${NAME} INTERFACE )

   target_include_directories ( ${NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
   target_sources( ${NAME} INTERFACE ${SOURCES})
   target_link_libraries( ${NAME} INTERFACE ${IMPORT_TARGETS} )
   target_compile_definitions( ${NAME} INTERFACE ${ADDITIONAL_DEFINES} )

   # define an additional interface library target
   make_interface_alias(${NAME} "SHARED") 
  
   list( APPEND TENACITY_LIBRARIES "${NAME}" )
   set( TENACITY_LIBRARIES "${TENACITY_LIBRARIES}" PARENT_SCOPE )
endmacro()

# The list of modules is ordered so that each module occurs after any others
# that it depends on
macro( tenacity_module_subdirectory modules )
   foreach( MODULE ${MODULES} )
      add_subdirectory("${MODULE}")
   endforeach()
   #propagate collected edges up to root CMakeLists.txt
   set( GRAPH_EDGES "${GRAPH_EDGES}" PARENT_SCOPE )
endmacro()
