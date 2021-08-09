#[[
A library of utilities relating generally to files and file names, including
an exception type for file errors.

Also the definition of certain significant file paths, such as the directory
for temporary projects.

Also the global logger which can save to a file.
]]#

set( SOURCES
   SaucedacityLogger.cpp
   SaucedacityLogger.h
   FileException.cpp
   FileException.h
   FileIO.cpp
   FileIO.h
   FileNames.cpp
   FileNames.h
   PlatformCompatibility.cpp
   PlatformCompatibility.h
   TempDirectory.cpp
   TempDirectory.h
   wxFileNameWrapper.h
)

set( LIBRARIES
   lib-exceptions-interface
   lib-preferences-interface
   PRIVATE
   wxWidgets::wxWidgets
)

saucedacity_library( lib-files "${SOURCES}" "${LIBRARIES}"
   "" ""
)
