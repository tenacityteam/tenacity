set(SOURCES
    audacityproject.dtd

    XMLFileReader.cpp
    XMLFileReader.h
    XMLFileWriter.cpp
    XMLFileWriter.h
    XMLTagHandler.cpp
    XMLTagHandler.h
    XMLStringWriter.cpp
    XMLStringWriter.h
    XMLWriter.cpp
    XMLWriter.h
)

set(LIBRARIES
    PRIVATE
        EXPAT::EXPAT
        wxWidgets::wxWidgets
        lib-files
        lib-math
        lib-strings
)

tenacity_library( lib-xml "${SOURCES}" "${LIBRARIES}" "" "")
