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

    execute_process(
        COMMAND
            xcrun notarytool submit "${path}"
                ${credentials}
                --wait
                --output-format json
        OUTPUT_VARIABLE result
    )

    # --wait returns once Apple finishes processing whether the upload was
    # accepted or not, so go by the status it reports. Auth and network
    # errors print to stderr instead of JSON and land here as NOTFOUND.
    string( JSON status ERROR_VARIABLE json_error GET "${result}" status )
    string( JSON id ERROR_VARIABLE json_error GET "${result}" id )

    message( STATUS "\t Submission ID: '${id}'" )
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
