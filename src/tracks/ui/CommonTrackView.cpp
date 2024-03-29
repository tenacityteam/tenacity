/**********************************************************************

Audacity: A Digital Audio Editor

CommonTrackView.cpp

Paul Licameli split from class TrackView

**********************************************************************/

#include "CommonTrackView.h"

#include "BackgroundCell.h"
#include "TimeShiftHandle.h"
#include "TrackControls.h"
#include "ZoomHandle.h"
#include "../ui/SelectHandle.h"
#include "../../AColor.h"
#include "../../ProjectSettings.h"
#include "Track.h"
#include "../../TrackArtist.h"
#include "../../TrackInfo.h"
#include "../../TrackPanelDrawingContext.h"
#include "../../TrackPanelMouseEvent.h"

#include <wx/dc.h>
#include <wx/graphics.h>

std::vector<UIHandlePtr> CommonTrackView::HitTest
(const TrackPanelMouseState &st,
 const TenacityProject *pProject)
{
   UIHandlePtr result;
   using namespace ToolCodes;
   std::vector<UIHandlePtr> results;
   const auto &settings = ProjectSettings::Get( *pProject );
   const auto currentTool = settings.GetTool();
   const bool isMultiTool = ( currentTool == multiTool );

   if ( !isMultiTool && currentTool == zoomTool ) {
      // Zoom tool is a non-selecting tool that takes precedence in all tracks
      // over all other tools, no matter what detail you point at.
      result = ZoomHandle::HitAnywhere(
         BackgroundCell::Get( *pProject ).mZoomHandle );
      results.push_back(result);
      return results;
   }

   // In other tools, let subclasses determine detailed hits.
   results =
      DetailedHitTest( st, pProject, currentTool, isMultiTool );

   // There are still some general cases.

   // Let the multi-tool right-click handler apply only in default of all
   // other detailed hits.
   if ( isMultiTool ) {
      result = ZoomHandle::HitTest(
         BackgroundCell::Get( *pProject ).mZoomHandle, st.state);
      if (result)
         results.push_back(result);
   }

   // Finally, default of all is adjustment of the selection box.
   if ( isMultiTool || currentTool == selectTool ) {
      result = SelectHandle::HitTest(
         mSelectHandle, st, pProject, shared_from_this() );
      if (result)
         results.push_back(result);
   }

   return results;
}

std::shared_ptr<TrackPanelCell> CommonTrackView::ContextMenuDelegate()
{
   return TrackControls::Get( *FindTrack() ).shared_from_this();
}

int CommonTrackView::GetMinimizedHeight() const
{
   auto height = TrackInfo::MinimumTrackHeight();
   const auto pTrack = FindTrack();
   auto channels = TrackList::Channels(pTrack->SubstituteOriginalTrack().get());
   auto nChannels = channels.size();
   auto begin = channels.begin();
   auto index =
      std::distance(begin, std::find(begin, channels.end(), pTrack.get()));
   return (height * (index + 1) / nChannels) - (height * index / nChannels);
}

#include "Envelope.h"
#include "ZoomInfo.h"
void CommonTrackView::GetEnvelopeValues( const Envelope &env,
   double alignedTime, double sampleDur,
   double *buffer, int bufferLen, int leftOffset,
   const ZoomInfo &zoomInfo )
{
   // Getting many envelope values, corresponding to pixel columns, which may
   // not be uniformly spaced in time when there is a fisheye.

   double prevDiscreteTime=0.0, prevSampleVal=0.0, nextSampleVal=0.0;
   for ( int xx = 0; xx < bufferLen; ++xx ) {
      auto time = zoomInfo.PositionToTime( xx, -leftOffset );
      if ( sampleDur <= 0 )
         // Sample interval not defined (as for time track)
         buffer[xx] = env.GetValue( time );
      else {
         // The level of zoom-in may resolve individual samples.
         // If so, then instead of evaluating the envelope directly,
         // we draw a piecewise curve with knees at each sample time.
         // This actually makes clearer what happens as you drag envelope
         // points and make discontinuities.
         auto leftDiscreteTime = alignedTime +
            sampleDur * floor( ( time - alignedTime ) / sampleDur );
         if ( xx == 0 || leftDiscreteTime != prevDiscreteTime ) {
            prevDiscreteTime = leftDiscreteTime;
            prevSampleVal =
               env.GetValue( prevDiscreteTime, sampleDur );
            nextSampleVal =
               env.GetValue( prevDiscreteTime + sampleDur, sampleDur );
         }
         auto ratio = ( time - leftDiscreteTime ) / sampleDur;
         if ( env.GetExponential() )
            buffer[ xx ] = exp(
               ( 1.0 - ratio ) * log( prevSampleVal )
                  + ratio * log( nextSampleVal ) );
         else
            buffer[ xx ] =
               ( 1.0 - ratio ) * prevSampleVal + ratio * nextSampleVal;
      }
   }
}
