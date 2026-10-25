# Code signing

if( CMAKE_SYSTEM_NAME MATCHES "Windows" )
   # On windows, we expect WINDOWS_CERTIFICATE to be set
   # as a CMake variable or as an environment variable
   if( DEFINED WINDOWS_CERTIFICATE )
      install( CODE "set( WINDOWS_CERTIFICATE \"${WINDOWS_CERTIFICATE}\" )" )
   endif()

   if( DEFINED WINDOWS_CERTIFICATE_PASSWORD )
      # To simplify the helper script - we push password to the environment
      install( CODE "set( ENV{WINDOWS_CERTIFICATE_PASSWORD} \"${WINDOWS_CERTIFICATE_PASSWORD}\") " )
   endif()

   install( CODE "set( PFX_SIGN_PS_LOCATION \"${CMAKE_SOURCE_DIR}/scripts/build/windows/PfxSign.ps1\") " )
   install( SCRIPT "scripts/build/windows/PfxSign.cmake" )
elseif( CMAKE_SYSTEM_NAME MATCHES "Darwin")
   set_from_env( APPLE_CODESIGN_IDENTITY )
   set_from_env( APPLE_NOTARIZATION_KEYCHAIN_PROFILE )
   set_from_env( APPLE_NOTARIZATION_USER_NAME )
   set_from_env( APPLE_NOTARIZATION_PASSWORD )
   set_from_env( APPLE_NOTARIZATION_TEAM_ID )

   # Without an identity every codesign call fails at install time, so bail
   # out during configure instead of after the whole build has finished.
   if( NOT APPLE_CODESIGN_IDENTITY )
      message( FATAL_ERROR
         "PERFORM_CODESIGN needs APPLE_CODESIGN_IDENTITY, e.g. "
         "\"Developer ID Application: Your Name (TEAMID)\". "
         "List yours with: security find-identity -v -p codesigning" )
   endif()

   # Pass arguments to cmake install script
   install( CODE "set( APPLE_CODESIGN_IDENTITY \"${APPLE_CODESIGN_IDENTITY}\" )" )
   install( CODE "set( APPLE_NOTARIZATION_KEYCHAIN_PROFILE \"${APPLE_NOTARIZATION_KEYCHAIN_PROFILE}\" )" )
   install( CODE "set( APPLE_NOTARIZATION_USER_NAME \"${APPLE_NOTARIZATION_USER_NAME}\" )" )
   install( CODE "set( APPLE_NOTARIZATION_PASSWORD \"${APPLE_NOTARIZATION_PASSWORD}\" )" )
   install( CODE "set( APPLE_NOTARIZATION_TEAM_ID \"${APPLE_NOTARIZATION_TEAM_ID}\" )" )

   install( CODE "get_filename_component( APP_LOCATION \${CMAKE_INSTALL_PREFIX}/Tenacity.app ABSOLUTE )" )
   install( CODE "set( APPLE_CODESIGN_ENTITLEMENTS ${CMAKE_SOURCE_DIR}/mac/Tenacity.entitlements )")

   install( SCRIPT "scripts/build/macOS/SignMacos.cmake" )

   if( PERFORM_NOTARIZATION )
      install( SCRIPT "scripts/build/macOS/NotarizeMacos.cmake" )
   endif()
endif()
