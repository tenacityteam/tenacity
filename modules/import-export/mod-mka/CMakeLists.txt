#[[
Matroska import and export
#]]

set(TARGET mod-mka)

set(SOURCES
    ImportMka.cpp
    Mka.cpp
)

set(LIBRARIES
    PRIVATE
        Matroska::matroska
        $<$<BOOL:${USE_LIBFLAC}>:FLAC::FLAC>
        $<$<BOOL:${USE_LIBFLAC}>:FLAC++::FLAC++>
)

set(EXTRA_CLUSTER_NODES "${LIBRARIES}" PARENT_SCOPE)

list(APPEND LIBRARIES
    lib-import-export-interface
    lib-label-track-interface
)

tenacity_module(${TARGET} "${SOURCES}" "${LIBRARIES}" "" "")
