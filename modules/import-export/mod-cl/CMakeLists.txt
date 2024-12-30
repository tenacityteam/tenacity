#[[
Export of files with a command passed to a command-line shell
]]

set( TARGET mod-cl )

set( SOURCES
      ExportCL.cpp
      CL.cpp
)

set( LIBRARIES
   PRIVATE
      Tenacity
)

if ( USE_LIBID3TAG )
      list ( APPEND LIBRARIES PRIVATE id3tag::id3tag)
endif()

set (EXTRA_CLUSTER_NODES "${LIBRARIES}" PARENT_SCOPE)

tenacity_module( ${TARGET} "${SOURCES}" "${LIBRARIES}" "" "" )
