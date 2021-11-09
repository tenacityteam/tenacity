set(TARGET lib-string-utils)
set(TARGET_ROOT ${CMAKE_CURRENT_SOURCE_DIR} )

def_vars()

set(SOURCES
    CodeConversions.h
    CodeConversions.cpp
    ToChars.h
    ToChars.cpp
)

set( LIBRARIES
   PRIVATE
   wxWidgets::wxWidgets
)

tenacity_library( ${TARGET} "${SOURCES}" "${LIBRARIES}" "" "" )
