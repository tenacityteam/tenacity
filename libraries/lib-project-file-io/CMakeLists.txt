#[[
A library handling project persistency, and supplying concrete SampleBlock
and SampleBlockFactory subclasses so that lib-wave-track is usable.

All usage of Sqlite is encapsulated in this library.
]]

set( SOURCES
   ActiveProjects.cpp
   ActiveProjects.h
   DBConnection.cpp
   DBConnection.h
   ProjectFileIOExtension.cpp
   ProjectFileIOExtension.h
   ProjectFileIO.cpp
   ProjectFileIO.h
   ProjectSerializer.cpp
   ProjectSerializer.h
   SqliteSampleBlock.cpp
)

set( LIBRARIES
   lib-wave-track-interface
)

# Only this library needs sqlite, so make the dependency private
list( APPEND LIBRARIES
   PRIVATE
      lib-sqlite-helpers-interface
)

tenacity_library( lib-project-file-io "${SOURCES}" "${LIBRARIES}"
   ""
   ""
)