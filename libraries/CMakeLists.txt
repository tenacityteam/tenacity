# Include the libraries that we'll build

# The list of modules is ordered so that each library occurs after any others
# that it depends on
set( LIBRARIES
   lib-basic-ui
   lib-components
   lib-exceptions
   lib-ffmpeg-support
   lib-files
   lib-math
   lib-preferences
   lib-string-utils
   lib-registries
   lib-strings
   lib-track
   lib-theme
   lib-utility
   lib-xml
   lib-audio-devices
   lib-project
   lib-screen-geometry
   lib-project-rate
)

foreach( LIBRARY ${LIBRARIES} )
   add_subdirectory( "${LIBRARY}" )
endforeach()

set( GRAPH_EDGES "${GRAPH_EDGES}" PARENT_SCOPE )
set( TENACITY_LIBRARIES "${TENACITY_LIBRARIES}" PARENT_SCOPE )
