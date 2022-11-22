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

TENACITY_DLL_API void DoSelectTimeAndTracks(
   TenacityProject &project, bool bAllTime, bool bAllTracks);
TENACITY_DLL_API void SelectAllIfNone( TenacityProject &project );
TENACITY_DLL_API bool SelectAllIfNoneAndAllowed( TenacityProject &project );
TENACITY_DLL_API void SelectNone( TenacityProject &project );
TENACITY_DLL_API void DoListSelection(
   TenacityProject &project, Track *t,
   bool shift, bool ctrl, bool modifyState );
TENACITY_DLL_API void DoSelectAll( TenacityProject &project );
TENACITY_DLL_API void DoSelectAllAudio( TenacityProject &project );
TENACITY_DLL_API void DoSelectSomething( TenacityProject &project );

}

#endif
