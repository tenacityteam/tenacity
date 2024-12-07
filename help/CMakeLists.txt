
set( TARGET manual )
set( TARGET_ROOT ${CMAKE_SOURCE_DIR}/manual )

message( STATUS "========== Configuring ${TARGET} ==========" )

def_vars()

if (NOT WIN32 AND NOT APPLE)
   configure_file( ${APP_NAME}.metainfo.xml.in ${_INTDIR}/${APP_NAME}.metainfo.xml )
endif()

add_custom_target( ${TARGET} DEPENDS "${MANUAL_PATH}" )

if (PACKAGE_MANUAL)
   # Error if the manual path is not empty and doesn't exist.
   if (NOT EXISTS "${MANUAL_PATH}")
      message(
         FATAL_ERROR
            "You have specified a manual path that doesn't exist. Check the path (${MANUAL_PATH}) and try again."
      )
   endif()

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
      install( DIRECTORY "${MANUAL_PATH}/" DESTINATION "${_APPDIR}/help/manual" )
   elseif( "${CMAKE_GENERATOR}" MATCHES "Visual Studio*" )
      install( DIRECTORY "${MANUAL_PATH}/" DESTINATION "help/manual" )
   endif()
endif()

if( NOT CMAKE_SYSTEM_NAME MATCHES "Darwin" )
   if( NOT "${CMAKE_GENERATOR}" MATCHES "Visual Studio*")
      install( DIRECTORY "${MANUAL_PATH}" OPTIONAL
               DESTINATION "${_DATADIR}/tenacity/help" )
      install( FILES "${_SRCDIR}/tenacity.1"
               DESTINATION "${_MANDIR}/man1" )
      install( FILES "${_INTDIR}/${APP_NAME}.metainfo.xml"
               DESTINATION "${_DATADIR}/metainfo" )
   endif()
endif()
