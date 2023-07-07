#[[
A library linking PortAudio and sufficient for querying the existing audio
input/output devices and finding their preferred sample rates.

Also a place to store global settings related to the preferred device.

Also abstract class Meter for communicating buffers of samples for display
purposes.

Does not contain an audio engine.
]]#

set( SOURCES
   AudioIOBase.cpp
   AudioIOBase.h
   AudioMemoryManager.cpp
   AudioMemoryManager.h
   DeviceChange.cpp
   DeviceChange.h
   DeviceManager.cpp
   DeviceManager.h
   Meter.cpp
   Meter.h
)
set( LIBRARIES
   PortAudio::PortAudio
   $<$<BOOL:${USE_MIDI}>:PortMidi::PortMidi>
   lib-preferences-interface
   PRIVATE
   wxWidgets::wxWidgets
)
tenacity_library( lib-audio-devices "${SOURCES}" "${LIBRARIES}"
   "" ""
)
