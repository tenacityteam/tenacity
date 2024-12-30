#[[
Extends Track with subclasses defining the notions of mute and solo.
]]

set( SOURCES
   PlayableTrack.cpp
   PlayableTrack.h
)
set( LIBRARIES
   lib-track-interface
)
tenacity_library( lib-playable-track "${SOURCES}" "${LIBRARIES}"
   "" ""
)
