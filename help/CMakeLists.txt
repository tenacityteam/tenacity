
set( TARGET manual )
set( TARGET_ROOT ${CMAKE_SOURCE_DIR}/manual )

message( STATUS "========== Configuring ${TARGET} ==========" )

def_vars()

set( src "${CMAKE_CURRENT_BINARY_DIR}/manual" )
set( dst "${_DEST}/help/manual" )
set( out_dir "${_INTDIR}" )
set( out "${out_dir}/${host}/index.html" )

if (NOT WIN32 AND NOT APPLE)
   configure_file( ${APP_NAME}.metainfo.xml.in ${_INTDIR}/${APP_NAME}.metainfo.xml )
endif()

add_custom_target( ${TARGET} DEPENDS "${out}" )

if( package_manual )
   install(CODE "                                     \
      execute_process(                                \
         COMMAND                                      \
            ${CMAKE_COMMAND}                          \
               --build ${CMAKE_BINARY_DIR}            \
               --config \${CMAKE_INSTALL_CONFIG_NAME} \
               --target manual                        \
      )                                               \
   ")

   if( "${CMAKE_GENERATOR}" MATCHES "Xcode" )
      install( DIRECTORY "${src}/" DESTINATION "${_APPDIR}/help/manual" )
   elseif( "${CMAKE_GENERATOR}" MATCHES "Visual Studio*" )
      install( DIRECTORY "${src}/" DESTINATION "help/manual" )
   endif()
endif()

if( NOT CMAKE_SYSTEM_NAME MATCHES "Darwin" )
   if( NOT WIN32)
      install( DIRECTORY "${dst}" OPTIONAL
               DESTINATION "${_DATADIR}/tenacity/help" )
      install( FILES "${_SRCDIR}/tenacity.1"
               DESTINATION "${_MANDIR}/man1" )
      install( FILES "${_INTDIR}/${APP_NAME}.metainfo.xml"
               DESTINATION "${_DATADIR}/metainfo" )
   endif()
endif()
