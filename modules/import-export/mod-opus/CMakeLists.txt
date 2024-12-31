#[[
Opus import and export
]]

set( TARGET mod-opus )

set( SOURCES
      ImportOpus.cpp
      ExportOpus.cpp
      Opus.cpp
)

set( LIBRARIES
   PRIVATE
      Opus::Opus
      OpusFile::OpusFile
      Ogg::ogg
)

set (EXTRA_CLUSTER_NODES "${LIBRARIES}" PARENT_SCOPE)

list(APPEND LIBRARIES
   lib-import-export-interface
)

tenacity_module( ${TARGET} "${SOURCES}" "${LIBRARIES}" "" "" )
