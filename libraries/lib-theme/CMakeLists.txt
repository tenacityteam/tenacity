#[[
A library to hold a registry of image and color identifiers, and multiple themes
associating data with those identifiers.
]]

set( SOURCES
   AColor.cpp
   AColor.h
   AColorResources.h
   AllThemeResources.cpp
   AllThemeResources.h
   ImageManipulation.cpp
   ImageManipulation.h
   MacroMagic.h
   ThemeLegacy.cpp
   ThemeLegacy.h

   # Rewritten theme system
   Theme.cpp
   Theme.h
   ThemePackage.cpp
   ThemePackage.h
   ThemeResourceList.cpp
   ThemeResourceList.h
   ThemeResources.cpp
   ThemeResources.h
   Types.h

   exceptions/ArchiveError.cpp
   exceptions/ArchiveError.h
   exceptions/IncompatibleTheme.cpp
   exceptions/IncompatibleTheme.h
   exceptions/InvalidState.cpp
   exceptions/InvalidState.h
)

set( LIBRARIES
   lib-files-interface
   libzip::zip
   wxWidgets::wxWidgets
 )

tenacity_library( lib-theme "${SOURCES}" "${LIBRARIES}"
   "" ""
)
