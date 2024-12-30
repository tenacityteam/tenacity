#[[
Ladspa effect plugin discovery and processing logic, without user interface
]]

if( USE_LADSPA )

set( SOURCES
   LadspaEffectBase.cpp
   LadspaEffectBase.h
   LadspaEffectsModule.cpp
   LadspaEffectsModule.h
   LadspaInstance.cpp
   LadspaInstance.h
   ladspa.h
)
set( LIBRARIES
   lib-effects-interface
)
tenacity_library( lib-ladspa "${SOURCES}" "${LIBRARIES}"
   "" ""
)

endif()
