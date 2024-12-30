#[[
A track that holds MIDI data.  Playback of MIDI.
]]

set( SOURCES
   MIDIPlay.cpp
   MIDIPlay.h
   NoteTrack.cpp
   NoteTrack.h
   WrapAllegro.h
)
set( LIBRARIES
   lib-audio-io-interface
   lib-playable-track-interface
   PortMidi::PortMidi
   PortSMF::PortSMF
)
tenacity_library( lib-note-track "${SOURCES}" "${LIBRARIES}"
   "" ""
)
