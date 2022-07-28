/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 SelectUtilities.h
 
 Paul Licameli split from SelectMenus.h
 
 **********************************************************************/

#ifndef __AUDACITY_SELECT_UTILITIES__
#define __AUDACITY_SELECT_UTILITIES__

class SaucedacityProject;
class Track;

/// Namespace for functions for Select menu
namespace SelectUtilities {

SAUCEDACITY_DLL_API void DoSelectTimeAndTracks(
   SaucedacityProject &project, bool bAllTime, bool bAllTracks);
SAUCEDACITY_DLL_API void SelectAllIfNone( SaucedacityProject &project );
SAUCEDACITY_DLL_API bool SelectAllIfNoneAndAllowed( SaucedacityProject &project );
SAUCEDACITY_DLL_API void SelectNone( SaucedacityProject &project );
SAUCEDACITY_DLL_API void DoListSelection(
   SaucedacityProject &project, Track *t,
   bool shift, bool ctrl, bool modifyState );
SAUCEDACITY_DLL_API void DoSelectAll( SaucedacityProject &project );
SAUCEDACITY_DLL_API void DoSelectAllAudio( SaucedacityProject &project );
SAUCEDACITY_DLL_API void DoSelectSomething( SaucedacityProject &project );

}

#endif
