set(SOURCES
    Theme.cpp
    Theme.h
    ThemePackage.cpp
    ThemePackage.h
    ThemeResources.cpp
    ThemeResources.h
    ThemeResourceList.cpp
    ThemeResourceList.h

    # Exceptions
    exceptions/ArchiveError.cpp
    exceptions/ArchiveError.h
    exceptions/IncompatibleTheme.cpp
    exceptions/IncompatibleTheme.h
)

set(LIBRARIES
    PUBLIC
        # JsonCpp must be public or else 'json/value.h' isn't found for some reason.
        JsonCpp::JsonCpp
    PRIVATE
        libzip::zip
)

tenacity_library(lib-theme "${SOURCES}" "${LIBRARIES}" "" "")
