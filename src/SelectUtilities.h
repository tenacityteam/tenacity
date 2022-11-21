/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 SelectUtilities.h
 
 Paul Licameli split from SelectMenus.h
 
 **********************************************************************/

#ifndef __AUDACITY_SELECT_UTILITIES__
#define __AUDACITY_SELECT_UTILITIES__

class TenacityProject;
class Track;

/// Namespace for functions for Select menu
namespace SelectUtilities {

SAUCEDACITY_DLL_API void DoSelectTimeAndTracks(
   TenacityProject &project, bool bAllTime, bool bAllTracks);
SAUCEDACITY_DLL_API void SelectAllIfNone( TenacityProject &project );
SAUCEDACITY_DLL_API bool SelectAllIfNoneAndAllowed( TenacityProject &project );
SAUCEDACITY_DLL_API void SelectNone( TenacityProject &project );
SAUCEDACITY_DLL_API void DoListSelection(
   TenacityProject &project, Track *t,
   bool shift, bool ctrl, bool modifyState );
SAUCEDACITY_DLL_API void DoSelectAll( TenacityProject &project );
SAUCEDACITY_DLL_API void DoSelectAllAudio( TenacityProject &project );
SAUCEDACITY_DLL_API void DoSelectSomething( TenacityProject &project );

}

#endif
