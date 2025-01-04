#[[
A directory of directories of module targets.  Sub-directories group modules in
some common area of functionality, such as import/export, and each group becomes
a cluster in the modules.dot file generated at configuration time.
]]

# Include the modules that we'll build

# The list of module sub-folders is ordered so that each folder occurs after any
# others that it depends on
set( FOLDERS
   etc
   import-export
   scripting
   nyquist
)

# Technically, based off the name, the track-ui directory shouldn't be bound by
# MIDI availability, but since it only contains mod-midi-import-export, we'll
# just exclude this entire folder to not build mod-midi-import-export.
#
# This can be changed if this becomes an issue.
if (USE_MIDI)
   list(APPEND FOLDERS track-ui)
endif()

foreach( FOLDER ${FOLDERS} )
   add_subdirectory("${FOLDER}")
endforeach()

#propagate collected edges and subgraphs up to root CMakeLists.txt
set( GRAPH_EDGES "${GRAPH_EDGES}" PARENT_SCOPE )
set( GRAPH_SUBGRAPHS "${GRAPH_SUBGRAPHS}" PARENT_SCOPE )
