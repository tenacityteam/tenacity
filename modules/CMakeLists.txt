# Include the modules that we'll build

# The list of modules is ordered so that each module occurs after any others
# that it depends on
set( MODULES
   mod-karaoke
   mod-script-pipe
)
if( NOT CMAKE_SYSTEM_NAME MATCHES "Windows" )
   list( APPEND MODULES
      mod-null
   )
endif()

foreach( MODULE ${MODULES} )
   add_subdirectory("${MODULE}")
endforeach()

if( NOT CMAKE_SYSTEM_NAME MATCHES "Darwin" )
   if( NOT "${CMAKE_GENERATOR}" MATCHES "Visual Studio*")
      install( DIRECTORY "${_DEST}/modules"
               DESTINATION "${_PKGLIB}" )
   endif()
endif()

#propagate collected edges up to root CMakeLists.txt
set( GRAPH_EDGES "${GRAPH_EDGES}" PARENT_SCOPE )
