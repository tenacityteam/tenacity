# CMake script to sign macOS build
# Arguments:
# APP_LOCATION - the path to Tenacity.app
# DMG_LOCATION - the path to Tenacity dmg package
# APPLE_CODESIGN_IDENTITY - identity to use
# APPLE_CODESIGN_ENTITLEMENTS - path to the entitlements

function( codesign path )
    message( STATUS "Signing ${path}" )

    execute_process(
        COMMAND
            xcrun codesign
                --force
                --verbose
                --timestamp
                --sign "${APPLE_CODESIGN_IDENTITY}"
                ${ARGN}
                "${path}"
        RESULT_VARIABLE result
    )

    if( NOT result EQUAL 0 )
        message( FATAL_ERROR "Failed to sign ${path}" )
    endif()
endfunction()

if( DEFINED APP_LOCATION )
    # A missing bundle used to fall through to codesign errors that nothing
    # checked, so the install "succeeded" with nothing signed.
    if( NOT IS_DIRECTORY "${APP_LOCATION}" )
        message( FATAL_ERROR "Cannot sign '${APP_LOCATION}': app bundle not found" )
    endif()

    # Sign inside-out instead of using --deep. --deep stamps the app's
    # identifier and entitlements onto every nested dylib, which Apple
    # advises against, and codesign needs nested code signed before the
    # bundle that contains it anyway.
    file( GLOB_RECURSE libraries
          LIST_DIRECTORIES Off
          "${APP_LOCATION}/Contents/*.dylib" "${APP_LOCATION}/Contents/*.so"
        )

    foreach( library ${libraries} )
        # vcpkg ships versioned dylib symlinks (libfoo.1.dylib ->
        # libfoo.1.2.3.dylib). The real file gets its own pass, so skip the
        # links rather than signing the same binary twice.
        if( NOT IS_SYMLINK "${library}" )
            codesign( "${library}" --options runtime )
        endif()
    endforeach()

    # GLOB_RECURSE with LIST_DIRECTORIES On returns every directory it walks
    # through, not just the ones matching the pattern, so it hands back
    # Contents/Frameworks, Contents/Resources, every .lproj, etc. Filter on
    # the name ourselves or codesign chokes on "bundle format unrecognized".
    file( GLOB_RECURSE frameworks
          LIST_DIRECTORIES On
          "${APP_LOCATION}/Contents/*.framework"
        )

    foreach( framework ${frameworks} )
        if( framework MATCHES "\\.framework$"
            AND IS_DIRECTORY "${framework}"
            AND NOT IS_SYMLINK "${framework}" )
            codesign( "${framework}" --options runtime )
        endif()
    endforeach()

    # Anything in Contents/MacOS runs as its own process, so it needs the
    # hardened runtime and the entitlements, not just the main binary.
    file( GLOB executables
          LIST_DIRECTORIES Off
          "${APP_LOCATION}/Contents/MacOS/*"
        )

    foreach( executable ${executables} )
        codesign( "${executable}"
            --options runtime
            --entitlements "${APPLE_CODESIGN_ENTITLEMENTS}"
        )
    endforeach()

    codesign( "${APP_LOCATION}"
        --options runtime
        --entitlements "${APPLE_CODESIGN_ENTITLEMENTS}"
    )

    # Catch anything the globs missed here, since notarytool takes several
    # minutes to tell you the same thing.
    execute_process(
        COMMAND xcrun codesign --verify --deep --strict --verbose=2 "${APP_LOCATION}"
        RESULT_VARIABLE result
    )

    if( NOT result EQUAL 0 )
        message( FATAL_ERROR "Signature verification failed for ${APP_LOCATION}" )
    endif()
endif()

if( DEFINED DMG_LOCATION )
    codesign( "${DMG_LOCATION}" )
endif()
