list( APPEND SOURCES
   ProjectRate.cpp
   ProjectRate.h
   QualitySettings.cpp
   QualitySettings.h
)

set(LIBRARIES
   lib-audio-devices-interface
   lib-math-interface
   lib-project-interface
   lib-xml-interface
   PRIVATE
      wxWidgets::wxWidgets
)

tenacity_library( lib-project-rate "${SOURCES}" "${LIBRARIES}" "" "")
