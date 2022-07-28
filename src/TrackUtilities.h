/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 TrackUtilities.h
 
 Paul Licameli split from TrackMenus.h
 
 **********************************************************************/

#ifndef __AUDACITY_TRACK_UTILITIES__
#define __AUDACITY_TRACK_UTILITIES__

class SaucedacityProject;
class Track;

namespace TrackUtilities {

   enum MoveChoice {
      OnMoveUpID, OnMoveDownID, OnMoveTopID, OnMoveBottomID
   };
   /// Move a track up, down, to top or to bottom.
   SAUCEDACITY_DLL_API void DoMoveTrack(
      SaucedacityProject &project, Track* target, MoveChoice choice );
   // "exclusive" mute means mute the chosen track and unmute all others.
   SAUCEDACITY_DLL_API
   void DoTrackMute( SaucedacityProject &project, Track *pTrack, bool exclusive );
   // Type of solo (standard or simple) follows the set preference, unless
   // exclusive == true, which causes the opposite behavior.
   SAUCEDACITY_DLL_API
   void DoTrackSolo( SaucedacityProject &project, Track *pTrack, bool exclusive );
   SAUCEDACITY_DLL_API
   void DoRemoveTrack( SaucedacityProject &project, Track * toRemove );
   SAUCEDACITY_DLL_API
   void DoRemoveTracks( SaucedacityProject & );

}

#endif
