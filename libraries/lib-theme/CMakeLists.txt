set(SOURCES
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
    PRIVATE
        JsonCpp::JsonCpp
        libzip::zip
)

tenacity_library(lib-theme "${SOURCES}" "${LIBRARIES}" "" "")
