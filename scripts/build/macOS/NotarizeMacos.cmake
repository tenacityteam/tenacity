# CMake script to notarize macOS build
# Arguments:
# APP_LOCATION - the path to Tenacity.app
# DMG_LOCATION - the path to Tenacity dmg package
#
# Credentials, either:
# APPLE_NOTARIZATION_KEYCHAIN_PROFILE - profile saved with
#                                       `xcrun notarytool store-credentials`
# or all of:
# APPLE_NOTARIZATION_USER_NAME - Apple ID
# APPLE_NOTARIZATION_PASSWORD - app-specific password for the Apple ID
# APPLE_NOTARIZATION_TEAM_ID - developer team ID

# https://cmake.org/cmake/help/latest/policy/CMP0054.html
cmake_policy( SET CMP0054 NEW )
# https://cmake.org/cmake/help/latest/policy/CMP0011.html
cmake_policy( SET CMP0011 NEW )

# altool stopped accepting notarization uploads in Nov 2023, so this goes
# through notarytool. A keychain profile is the easy path on a dev Mac;
# CI runners don't have one, so they pass the Apple ID, app-specific
# password and team ID instead.
if( APPLE_NOTARIZATION_KEYCHAIN_PROFILE )
    set( credentials --keychain-profile "${APPLE_NOTARIZATION_KEYCHAIN_PROFILE}" )
elseif( APPLE_NOTARIZATION_USER_NAME AND APPLE_NOTARIZATION_PASSWORD AND APPLE_NOTARIZATION_TEAM_ID )
    set( credentials
        --apple-id "${APPLE_NOTARIZATION_USER_NAME}"
        --password "${APPLE_NOTARIZATION_PASSWORD}"
        --team-id "${APPLE_NOTARIZATION_TEAM_ID}"
    )
else()
    message( FATAL_ERROR
        "Notarization needs APPLE_NOTARIZATION_KEYCHAIN_PROFILE, or "
        "APPLE_NOTARIZATION_USER_NAME, APPLE_NOTARIZATION_PASSWORD and "
        "APPLE_NOTARIZATION_TEAM_ID. Create a profile with: "
        "xcrun notarytool store-credentials <name>" )
endif()

function( notarize path )
    message( STATUS "Notarizing ${path}" )

    # Submit and wait separately. With `submit --wait` the JSON we parse
    # only shows up at the very end, so the build sits silent for minutes
    # and looks hung, and if it gets killed partway you never learn the
    # submission ID. Auth and network errors print to stderr instead of
    # JSON and land here as NOTFOUND.
    execute_process(
        COMMAND
            xcrun notarytool submit "${path}"
                ${credentials}
                --output-format json
        OUTPUT_VARIABLE result
    )

    string( JSON id ERROR_VARIABLE json_error GET "${result}" id )

    if( NOT id )
        message( FATAL_ERROR "Failed to submit ${path} for notarization:\n${result}\n" )
    endif()

    message( STATUS "\t Submission ID: '${id}'" )
    message( STATUS "\t Waiting on Apple, which usually takes a few minutes." )
    message( STATUS "\t If this gets interrupted, check on it later with:" )
    message( STATUS "\t   xcrun notarytool info ${id} <your notarytool credentials>" )

    # Leave wait's output uncaptured so its progress lines reach the log
    execute_process( COMMAND xcrun notarytool wait "${id}" ${credentials} )

    # wait returns once Apple finishes whether the upload was accepted or
    # not, so ask for the final status rather than trusting its exit code
    execute_process(
        COMMAND
            xcrun notarytool info "${id}"
                ${credentials}
                --output-format json
        OUTPUT_VARIABLE result
    )

    string( JSON status ERROR_VARIABLE json_error GET "${result}" status )

    message( STATUS "\t Status: ${status}" )

    if( NOT "${status}" STREQUAL "Accepted" )
        # The log is the only place Apple says which file it didn't like
        if( id )
            execute_process( COMMAND xcrun notarytool log "${id}" ${credentials} )
        endif()

        message( FATAL_ERROR "Notarization failed:\n${result}\n" )
    endif()
endfunction()

# Stapling bakes the ticket into the file so Gatekeeper doesn't have to
# look it up online the first time someone opens it.
function( staple path )
    execute_process(
        COMMAND xcrun stapler staple "${path}"
        RESULT_VARIABLE result
    )

    if( NOT result EQUAL 0 )
        message( FATAL_ERROR "Failed to staple notarization ticket to ${path}" )
    endif()
endfunction()

# notarytool won't take a bare .app, so zip it with ditto (which keeps the
# bundle's symlinks and extended attributes intact), notarize the zip, and
# staple the ticket to the app itself.
if( DEFINED APP_LOCATION )
    get_filename_component( archive "${APP_LOCATION}/../notarization.zip" ABSOLUTE )

    execute_process(
        COMMAND
            xcrun ditto
                -c -k --keepParent
                "${APP_LOCATION}"
                "${archive}"
    )

    notarize( "${archive}" )
    staple( "${APP_LOCATION}" )

    file( REMOVE "${archive}" )
endif()

if( DEFINED DMG_LOCATION )
    notarize( "${DMG_LOCATION}" )
    staple( "${DMG_LOCATION}" )
endif()
