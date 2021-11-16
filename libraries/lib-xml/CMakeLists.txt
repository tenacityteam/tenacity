#[[
   Utilities to serialize and deserialize trees of objects in XML form, and a
   class template XMLMethodRegistry to generate registries for serializable
   objects attached to a host object.  The template is parametrized by the host
   type.
]]#

set(SOURCES
    audacityproject.dtd

    XMLAttributeValueView.cpp
    XMLAttributeValueView.h 
    XMLFileReader.cpp
    XMLFileReader.h
    XMLFileWriter.cpp
    XMLFileWriter.h
    XMLMethodRegistry.cpp
    XMLMethodRegistry.h
    XMLTagHandler.cpp
    XMLTagHandler.h
    XMLStringWriter.cpp
    XMLStringWriter.h
    XMLWriter.cpp
    XMLWriter.h
)

set(LIBRARIES
        EXPAT::EXPAT
        lib-files-interface
        lib-string-utils-interface
    PRIVATE
        wxWidgets::wxWidgets
)

tenacity_library( lib-xml "${SOURCES}" "${LIBRARIES}" "" "")
