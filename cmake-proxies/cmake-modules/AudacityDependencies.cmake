
macro ( find_required_package package_name system_package_name )
    find_package ( ${package_name} QUIET ${ARGN} )

    if ( NOT ${package_name}_FOUND )
        if (CMAKE_SYSTEM_NAME MATCHES "Darwin|Windows")
            message( FATAL_ERROR "Error: ${package_name} is required")
        else()
            message( FATAL_ERROR "Error: ${package_name} is required.\nPlease install it with using command like:\n\t\$ sudo apt install ${system_package_name}" )
        endif()
    endif()
endmacro()
