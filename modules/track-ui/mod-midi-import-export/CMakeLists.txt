#[[
Adds MIDI import and export, using the portsmf library
]]

set( SOURCES
   ExportMIDI.cpp
   ImportMIDI.cpp
)
set( LIBRARIES PRIVATE
   Tenacity
)
tenacity_module( mod-midi-import-export "${SOURCES}" "${LIBRARIES}"
   "" "" )
