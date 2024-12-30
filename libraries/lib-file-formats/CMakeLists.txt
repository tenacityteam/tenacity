#[[
Provides audio file information via libsndfile
]]

set( SOURCES
   FileFormats.cpp
   FileFormats.h
   AcidizerTags.h
)
set( LIBRARIES
   PUBLIC
      SndFile::sndfile
      lib-math-interface
)
tenacity_library( lib-file-formats "${SOURCES}" "${LIBRARIES}"
   "" ""
)
